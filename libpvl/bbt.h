/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A boolean perfect binary tree
 */
#include <stddef.h>

struct bool_binary_tree;

struct bool_binary_tree_pos;

/* Returns the sizeof of a bbt of the desired order. Keep order small. */
size_t bool_binary_tree_sizeof(unsigned char order);

/* Initializes a boolean binary tree at the specified location. Keep order small. */
struct bool_binary_tree *bool_binary_tree_init(char *at, unsigned char order);

/* Sets a position at the specified depth */
void *at_depth(struct bool_binary_tree *t, size_t depth, struct bool_binary_tree_pos *pos);

/* Navigates to the left child node. Returns false if this is a leaf node. */
_Bool left_child(struct bool_binary_tree_pos *pos);

/* Navigates to the right child node. Returns false if this is a leaf node. */
_Bool right_child(struct bool_binary_tree_pos *pos);

/* Navigates to the left adjacent node. Returns false if there is no left adjacent node. */
_Bool left_adjacent(struct bool_binary_tree_pos *pos);

/* Navigates to the right adjacent node. Returns false if there is no right adjacent node. */
_Bool right_adjacent(struct bool_binary_tree_pos *pos);

/* Navigates to the patern node. Returns false if this is the root node. */
_Bool parent(struct bool_binary_tree_pos *pos);

/* Set the value at the indicated position */
void set(struct bool_binary_tree_pos *pos);

/* Clear the value at the indicated  position */
void clear(struct bool_binary_tree_pos *pos);

/* Flips the value at the indicated  position */
void flip(struct bool_binary_tree_pos *pos);

/* Returns the value at the indicated  position */
_Bool test(struct bool_binary_tree_pos *pos);

/* Returns the depth at the indicated  position */
size_t depth(struct bool_binary_tree_pos *pos);
