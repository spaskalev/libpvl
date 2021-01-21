/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A boolean perfect binary tree
 */
#include "bbt.h"
#include "bitset.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct bbt {
	unsigned char order;
	size_t elements_count;
	unsigned char bits[];
};

struct bbt_pos {
	struct bbt *t;
	size_t pos;
};

static size_t highest_bit_set(size_t value);

size_t bbt_sizeof(unsigned char order) {
	if ((order == 0) || (order >= (sizeof(size_t) * CHAR_BIT))) {
		/* return something wild to detect failure
		 * the init function will also check this
		 * and fail to initialize */
		return SIZE_MAX;
	}
	size_t result = sizeof(struct bbt);
	result += bitset_size(1u << order);
	return result;
}

size_t bbt_pos_sizeof() {
	return sizeof(struct bbt_pos);
}

struct bbt *bbt_init(unsigned char *at, unsigned char order) {
	size_t alignment = ((uintptr_t) at) % alignof(max_align_t);
	if (alignment != 0) {
		return NULL;
	}
	if ((order == 0) || (order >= (sizeof(size_t) * CHAR_BIT))) {
		return NULL;
	}
	struct bbt *t = (struct bbt*) at;
	t->order = order;
	t->elements_count = 1u << order;
	memset(t->bits, 0, bitset_size(t->elements_count));
	return t;
}

void bbt_pos_at_depth(struct bbt *t, size_t depth, struct bbt_pos *pos) {
	if (depth >= t->order) {
		return;
	}
	pos->t = t;
	pos->pos = 1;
	while (depth && bbt_pos_left_child(pos)) {
		depth--;
	}
}

_Bool bbt_pos_left_child(struct bbt_pos *pos) {
	if (bbt_pos_depth(pos) + 1 == pos->t->order) {
		return 0;
	}
	pos->pos = 2 * pos->pos;
	return 1;
}

_Bool bbt_pos_right_child(struct bbt_pos *pos) {
	if (bbt_pos_depth(pos) + 1 == pos->t->order) {
		return 0;
	}
	pos->pos = (2 * pos->pos) + 1;
	return 1;
}

_Bool bbt_pos_left_adjacent(struct bbt_pos *pos) {
	if (pos->pos <= 1) {
		return 0;
	}
	if (highest_bit_set(pos->pos) != highest_bit_set(pos->pos - 1)) {
		return 0;
	}
	pos->pos -= 1;
	return 1;
}

_Bool bbt_pos_right_adjacent(struct bbt_pos *pos) {
	if (pos->pos <= 1) {
		return 0;
	}
	if (pos->pos == pos->t->elements_count - 1) {
		return 0;
	}
	if (highest_bit_set(pos->pos) != highest_bit_set(pos->pos + 1)) {
		return 0;
	}
	pos->pos += 1;
	return 1;
}

_Bool bbt_pos_parent(struct bbt_pos *pos) {
	size_t parent = pos->pos / 2;
	if ((parent != pos->pos) && parent != 0) {
		pos->pos = parent;
		return 1;
	}
	return 0;
}

void bbt_pos_set(struct bbt_pos *pos) {
	bitset_set(pos->t->bits, pos->pos);
}

void bbt_pos_clear(struct bbt_pos *pos) {
	bitset_clear(pos->t->bits, pos->pos);
}

void bbt_pos_flip(struct bbt_pos *pos) {
	bitset_flip(pos->t->bits, pos->pos);
}

_Bool bbt_pos_test(struct bbt_pos *pos) {
	return bitset_test(pos->t->bits, pos->pos);
}

size_t bbt_pos_depth(struct bbt_pos *pos) {
	return highest_bit_set(pos->pos);
}

static size_t highest_bit_set(size_t value) {
	size_t pos = 0;
	while ( value >>= 1u ) {
		pos++;
	}
	return pos;
}
