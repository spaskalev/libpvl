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

struct bool_binary_tree {
	unsigned char order;
	size_t elements_count;
	unsigned char bits[];
};

struct bool_binary_tree_pos {
	struct bool_binary_tree *t;
	size_t pos;
};

static size_t highest_bit_set(size_t value);

size_t bool_binary_tree_sizeof(unsigned char order) {
	if ((order == 0) || (order >= (sizeof(size_t) * CHAR_BIT))) {
		/* return something wild to detect failure
		 * the init function will also check this
		 * and fail to initialize */
		return SIZE_MAX;
	}
	size_t result = sizeof(struct bool_binary_tree);
	result += bitset_size(1u << order);
	return result;
}

size_t bool_binary_tree_pos_sizeof() {
	return sizeof(struct bool_binary_tree_pos);
}

struct bool_binary_tree *bool_binary_tree_init(char *at, unsigned char order) {
	size_t alignment = ((uintptr_t) at) % alignof(max_align_t);
	if (alignment != 0) {
		return NULL;
	}
	if ((order == 0) || (order >= (sizeof(size_t) * CHAR_BIT))) {
		return NULL;
	}
	struct bool_binary_tree *t = (struct bool_binary_tree*) at;
	t->order = order;
	t->elements_count = 1u << order;
	memset(t->bits, 0, bitset_size(t->elements_count));
	return t;
}

void at_depth(struct bool_binary_tree *t, size_t depth, struct bool_binary_tree_pos *pos) {
	pos->t = t;
	pos->pos = 1;
	while (depth && left_child(pos)) {
		depth--;
	}
}

_Bool left_child(struct bool_binary_tree_pos *pos) {
	size_t left = 2 * pos->pos;
	if (left <= pos->t->elements_count) {
		pos->pos = left;
		return 1;
	}
	return 0;
}

_Bool right_child(struct bool_binary_tree_pos *pos) {
	size_t right = (2 * pos->pos) + 1;
	if (right <= pos->t->elements_count) {
		pos->pos = right;
		return 1;
	}
	return 0;
}

_Bool left_adjacent(struct bool_binary_tree_pos *pos) {
	if (pos->pos <= 1) {
		return 0;
	}
	if (highest_bit_set(pos->pos) != highest_bit_set(pos->pos - 1)) {
		return 0;
	}
	pos->pos -= 1;
	return 1;
}

_Bool right_adjacent(struct bool_binary_tree_pos *pos) {
	if (pos->pos == pos->t->elements_count) {
		return 0;
	}
	if (highest_bit_set(pos->pos) != highest_bit_set(pos->pos + 1)) {
		return 0;
	}
	pos->pos += 1;
	return 1;
}

_Bool parent(struct bool_binary_tree_pos *pos) {
	size_t parent = pos->pos / 2;
	if (parent != pos->pos) {
		pos->pos = parent;
		return 1;
	}
	return 0;
}

void set(struct bool_binary_tree_pos *pos) {
	bitset_set(pos->t->bits, pos->pos);
}

void clear(struct bool_binary_tree_pos *pos) {
	bitset_clear(pos->t->bits, pos->pos);
}

void flip(struct bool_binary_tree_pos *pos) {
	bitset_flip(pos->t->bits, pos->pos);
}

_Bool test(struct bool_binary_tree_pos *pos) {
	return bitset_test(pos->t->bits, pos->pos);
}

size_t depth(struct bool_binary_tree_pos *pos) {
	return highest_bit_set(pos->pos);
}

static size_t highest_bit_set(size_t value) {
	size_t pos = 0;
	while ( value >>= 1u ) {
		pos++;
	}
	return pos;
}
