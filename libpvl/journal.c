/*
 * Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * FILE journal handlers for libpvl (implementation)
 */

#include <stddef.h>
#include <stdio.h>
#include "journal.h"

int pvl_journal_write(void *ctx, void *from, size_t length, size_t remaining);
int pvl_journal_read(void *ctx, void *to, size_t length, size_t remaining);

int pvl_journal_write(void *ctx, void *from, size_t length, size_t remaining) {
	(void)(remaining);
	if (ctx == NULL) {
		return 1;
	}
	if (from == NULL) {
		return 1;
	}
	if (length == 0) {
		return 1;
	}
	struct pvl_journal_config *config = (struct pvl_journal_config*)(ctx);
	if (! config->destination) {
		return 1;
	}
	if (fwrite(from, length, 1, config->destination) != 1) {
		return 1;
	}
	return 0;
}

int pvl_journal_read(void *ctx, void *to, size_t length, size_t remaining) {
	if (ctx == NULL) {
		return 1;
	}
	struct pvl_journal_config *config = (struct pvl_journal_config*)(ctx);
	if (! config->destination) {
		return 1;
	}
	(void)(to);
	(void)(length);
	(void)(remaining);
	return 0;
}
