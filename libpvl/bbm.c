/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 */

#include "bbm.h"
#include "bbt.h"

#include <stddef.h>
#include <stdint.h>

struct bbm {
	unsigned char *main;
	size_t memory_size;
	size_t bbt_order;
	alignas(max_align_t) unsigned char bbt_backing[];
};

static size_t bbt_order_for_memory(size_t memory_size);
static size_t depth_for_size(struct bbm *bbm, size_t requested_size);
static size_t size_for_depth(struct bbm *bbm, size_t depth);

size_t bbm_sizeof(size_t memory_size) {
	if ((memory_size % BBM_ALIGN) != 0) {
		return 0; /* invalid */
	}
	if (memory_size == 0) {
		return 0; /* invalid */
	}
	size_t bbt_order = bbt_order_for_memory(memory_size);
	size_t bbt_size = bbt_sizeof(bbt_order);
	return sizeof(struct bbm) + bbt_size;
}

struct bbm *bbm_init(unsigned char *at, unsigned char *main, size_t memory_size) {
	if ((at == NULL) || (main == NULL)) {
		return NULL;
	}
	size_t at_alignment = ((uintptr_t) at) % alignof(max_align_t);
	if (at_alignment != 0) {
		return NULL;
	}
	size_t main_alignment = ((uintptr_t) main) % alignof(max_align_t);
	if (main_alignment != 0) {
		return NULL;
	}
	size_t size = bbm_sizeof(memory_size);
	if (size == 0) {
		return NULL;
	}
	size_t bbt_order = bbt_order_for_memory(memory_size);
	/* TODO check for overlap between bbm metadata and main block */
	struct bbm *bbm = (struct bbm *) at;
	bbm->main = main;
	bbm->memory_size = memory_size;
	bbt_init(bbm->bbt_backing, bbt_order);
	bbm->bbt_order = bbt_order;
	return bbm;
}

static size_t bbt_order_for_memory(size_t memory_size) {
	size_t blocks = memory_size / BBM_ALIGN;
	size_t bbt_order = 1;
	while (blocks >>= 1u) {
		bbt_order++;
	}
	return bbt_order;
}

void *bbm_malloc(struct bbm *bbm, size_t requested_size) {
	if (bbm == NULL) {
		return NULL;
	}
	if (requested_size == 0) {
		return NULL;
	}
	if (requested_size > bbm->memory_size) {
		return NULL;
	}
	size_t target_depth = depth_for_size(bbm, requested_size);
	struct bbt* bbt = (struct bbt*) bbm->bbt_backing;
	bbt_pos pos = bbt_left_pos_at_depth(bbt, target_depth);

	while(1) {
		if (bbt_pos_test(bbt, pos)) {
			/* branch is busy, try the next one */
			pos = bbt_pos_right_adjacent(bbt, pos);
			if(!bbt_pos_valid(bbt, pos)) {
				/* no more rightward positions */
				return NULL;
			}
			continue;
		}
		/* else */
		/*
		 * go upwards until first set position OR root position
		 *
		 * If all is unset all the way to root we're good.
		 * If a set position is found check previous sibling.
		 */
		bbt_pos previous;
		bbt_pos parent = pos;
		while (1) {
			previous = parent;
			parent = bbt_pos_parent(bbt, parent);
			if (bbt_pos_valid(bbt, parent)) {
				if (bbt_pos_test(bbt, parent)) {
					/* parent is marked, check previous sibling */
					if (bbt_pos_test(bbt, bbt_pos_sibling(bbt, previous))) {
						/* sibling is marked, so the currently-found slot is good */
						goto slot_found;
					}
					/* else */
					/*
					 * sibling is unmarked so this branch is used
					 * skip the branch and descend back to target depth
					 */
					 pos = bbt_pos_right_adjacent(bbt, parent);
					 if (!bbt_pos_valid(bbt, pos)) {
						 /* no more branches remain */
						 return NULL;
				     }
				     while (bbt_pos_depth(bbt, pos) != target_depth) {
						 pos = bbt_pos_left_child(bbt, pos);
					 }
					 break;
				}
				/* else */
				/* parent is free, go up */
				continue;
			}
			/* else */
			/* we've reached root - the current slot is good */
			goto slot_found;
		}
	}

	slot_found:;
	/* Find the return address */
	size_t block_size = size_for_depth(bbm, target_depth);
	size_t addr = block_size * bbt_pos_index(bbt, pos);

	/* Mark as allocated */
	bbt_pos_set(bbt, pos);
	while ((pos = bbt_pos_parent(bbt, pos))) {
		if (bbt_pos_test(bbt, pos)) {
			break;
		}
		bbt_pos_set(bbt, pos);
	}
	return (bbm->main + addr);
}

void bbm_free(struct bbm *bbm, void *ptr) {
	if (bbm == NULL) {
		return;
	}
	if (ptr == NULL) {
		return;
	}
	unsigned char *dst = (unsigned char *)ptr;
	if ((dst < bbm->main) || (dst > (bbm->main + bbm->memory_size))) {
		return;
	}

	/* Find the deepest pos tracking this address */
	ptrdiff_t offset = dst - bbm->main;
	size_t index = offset / BBM_ALIGN;
	struct bbt* bbt = (struct bbt*) bbm->bbt_backing;
	bbt_pos pos = bbt_left_pos_at_depth(bbt, bbm->bbt_order-1);
	while (index > 0) {
		pos = bbt_pos_right_adjacent(bbt, pos);
		index--;
	}

	/* Clear bits upward */
	while (!bbt_pos_test(bbt, pos)) {
		if (!(pos = bbt_pos_parent(bbt, pos))) {
			/* root reached and clear, nothing to unset */
			return;
		}
	}
	while (1) {
		bbt_pos_clear(bbt, pos);
		if (!(pos = bbt_pos_sibling(bbt, pos))) {
			return;
		}
		if (bbt_pos_test(bbt, pos)) {
			/* sibling allocated, can return */
			return;
		}
		pos = bbt_pos_parent(bbt, pos);
	}
}

static size_t depth_for_size(struct bbm *bbm, size_t requested_size) {
	size_t depth = 0;
	size_t memory_size = bbm->memory_size;
	while ((memory_size / requested_size) >> 1u) {
		depth++;
		memory_size >>= 1u;
	}
	return depth;
}

static size_t size_for_depth(struct bbm *bbm, size_t depth) {
	size_t result = bbm->memory_size >> depth;
	return result;
}

void bbm_debug_print(struct bbm *bbm) {
	struct bbt* bbt = (struct bbt*) bbm->bbt_backing;
	bbt_pos pos = bbt_left_pos_at_depth(bbt, 0);
	bbt_debug_pos_print(bbt, pos);
}
