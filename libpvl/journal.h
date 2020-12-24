/*
 * Copyright 2020 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * FILE journal handlers for libpvl
 */
#pragma once

#include <stddef.h>
#include <stdio.h>

struct pvl_journal_config {
	FILE* destination;
};

int pvl_journal_write(void *ctx, void *from, size_t length, size_t remaining);
int pvl_journal_read(void *ctx, void *to, size_t length, size_t remaining);
