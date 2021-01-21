/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A boolean perfect binary tree
 */
#include <stddef.h>

struct bbt;

struct bbt_pos;

/* Returns the sizeof of a bbt of the desired order. Keep order small. */
size_t bbt_sizeof(unsigned char order);

/* Returns the sizeof of the position handle */
size_t bbt_pos_sizeof();

/* Initializes a boolean binary tree at the specified location. Keep order small. */
struct bbt *bbt_init(char *at, unsigned char order);

/* Sets a position at the specified depth */
void bbt_pos_at_depth(struct bbt *t, size_t depth, struct bbt_pos *pos);

/* Navigates to the left child node. Returns false if this is a leaf node. */
_Bool bbt_pos_left_child(struct bbt_pos *pos);

/* Navigates to the right child node. Returns false if this is a leaf node. */
_Bool bbt_pos_right_child(struct bbt_pos *pos);

/* Navigates to the left adjacent node. Returns false if there is no left adjacent node. */
_Bool bbt_pos_left_adjacent(struct bbt_pos *pos);

/* Navigates to the right adjacent node. Returns false if there is no right adjacent node. */
_Bool bbt_pos_right_adjacent(struct bbt_pos *pos);

/* Navigates to the patern node. Returns false if this is the root node. */
_Bool bbt_pos_parent(struct bbt_pos *pos);

/* Set the value at the indicated position */
void bbt_pos_set(struct bbt_pos *pos);

/* Clear the value at the indicated  position */
void bbt_pos_clear(struct bbt_pos *pos);

/* Flips the value at the indicated  position */
void bbt_pos_flip(struct bbt_pos *pos);

/* Returns the value at the indicated  position */
_Bool bbt_pos_test(struct bbt_pos *pos);

/* Returns the depth at the indicated  position */
size_t bbt_pos_depth(struct bbt_pos *pos);
