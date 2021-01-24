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
	struct bbt *bbt;
	size_t bbt_order;
	alignas(max_align_t) unsigned char bbt_backing[];
};

static size_t bbt_order_for_memory(size_t memory_size);
static size_t depth_for_size(struct bbm *bbm, size_t requested_size);
static size_t size_for_depth(struct bbm *bbm, size_t depth);
bbt_pos search_free_slot(struct bbt *bbt, bbt_pos pos, size_t target_depth);

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
	bbm->bbt = bbt_init(bbm->bbt_backing, bbt_order);
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
	bbt_pos pos = bbt_left_pos_at_depth(bbm->bbt, 0);
	pos = search_free_slot(bbm->bbt, pos, target_depth);
	if (pos == 0) {
		return NULL;
	}
	/* Find the return address */
	size_t block_size = size_for_depth(bbm, target_depth);
	size_t addr = block_size * bbt_pos_index(bbm->bbt, &pos);
	/* Mark as allocated */
	bbt_pos_set(bbm->bbt, &pos);
	while (bbt_pos_parent(bbm->bbt, &pos)) {
		if (bbt_pos_test(bbm->bbt, &pos)) {
			break;
		}
		bbt_pos_set(bbm->bbt, &pos);
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
	bbt_pos pos = bbt_left_pos_at_depth(bbm->bbt, bbm->bbt_order-1);
	while (index > 0) {
		bbt_pos_right_adjacent(bbm->bbt, &pos);
		index--;
	}

	/* Clear bits upward */
	while (!bbt_pos_test(bbm->bbt, &pos)) {
		if (!bbt_pos_parent(bbm->bbt, &pos)) {
			/* root reached and clear, nothing to unset */
			return;
		}
	}
	while (1) {
		bbt_pos_clear(bbm->bbt, &pos);
		if (!bbt_pos_sibling(bbm->bbt, &pos)) {
			return;
		}
		if (bbt_pos_test(bbm->bbt, &pos)) {
			/* sibling allocated, can return */
			return;
		}
		bbt_pos_parent(bbm->bbt, &pos);
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

bbt_pos search_free_slot(struct bbt *bbt, bbt_pos pos, size_t target_depth) {
	size_t current_depth = bbt_pos_depth(bbt, &pos);
	if (bbt_pos_test(bbt, &pos)) {
		/* branch is allocated, return */
		if (current_depth == target_depth) {
			return 0;
		}
		bbt_pos left_child = pos;
		bbt_pos right_child = pos;
		bbt_pos_left_child(bbt, &left_child);
		bbt_pos_right_child(bbt, &right_child);
		if (bbt_pos_test(bbt, &left_child) || bbt_pos_test(bbt, &right_child)) {
			bbt_pos result = 0;
			result = search_free_slot(bbt, left_child, target_depth);
			if (result == 0) {
				result = search_free_slot(bbt, right_child, target_depth);
			}
			return result;
		}
		/* both children are unset which means this node is already allocated */
		return 0;
	}

	/* branch in free, terminate if target depth is reached */
	if (current_depth == target_depth) {
		return pos;
	}
	bbt_pos next = pos;
	bbt_pos_left_child(bbt, &next);
	return search_free_slot(bbt, next, target_depth);

	return 0;
}
