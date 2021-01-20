/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A char-backed bitset implementation
 */
#pragma once

#include <limits.h>
#include <stddef.h>

size_t bitset_size(size_t elements);

void bitset_set(unsigned char *bitset, size_t from_pos, size_t to_pos);

_Bool bitset_test(const unsigned char *bitset, size_t pos);
