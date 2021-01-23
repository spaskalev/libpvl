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

size_t bbm_sizeof(size_t memory_size) {
	if ((memory_size % BBM_ALIGN) != 0) {
		return 0; /* invalid */
	}
	if (memory_size == BBM_ALIGN) {
		return 0; /* invalid */
	}
	size_t bbt_order = bbt_order_for_memory(memory_size);
	size_t bbt_size = bbt_sizeof(bbt_order);
	if (bbt_size == 0) {
		return 0; /* invalid */
	}
	return sizeof(struct bbm) + bbt_size;
}

struct bbm *bbm_init(unsigned char *at, unsigned char *main, size_t memory_size) {
	size_t alignment = ((uintptr_t) at) % alignof(max_align_t);
	if (alignment != 0) {
		return NULL;
	}
	if ((at == NULL) || (main == NULL)) {
		return NULL;
	}
	size_t size = bbm_sizeof(memory_size);
	if (size == SIZE_MAX) {
		return NULL;
	}
	size_t bbt_order = bbt_order_for_memory(memory_size);
	/* TODO check for overlap between bbm metadata and main block */
	struct bbm *bbm = (struct bbm *) at;
	bbm->main = main;
	bbm->memory_size = memory_size;
	bbm->bbt = bbt_init(bbm->bbt_backing, bbt_order);
	if (bbm->bbt == NULL) {
		return NULL;
	}
	bbm->bbt_order = bbt_order;
	return bbm;
}

void *bbm_malloc(struct bbm *bbm, size_t size);

void bbm_free(struct bbm *bbm, void *ptr);

static size_t bbt_order_for_memory(size_t memory_size) {
	size_t blocks = memory_size / BBM_ALIGN;
	size_t bbt_order = 0;
	while (blocks >>= 1u) {
		bbt_order++;
	}
	return bbt_order;
}
