/*
 * Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tests.h"
#include "bitset.h"
#include "bbt.h"
#include "buddy_alloc_tree.h"
#include "buddy_alloc.h"
#include "pvl.h"

#define pvl_header_size (2*sizeof(size_t))

void test_init_misalignment() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)+1];
	char main_mem[1024];
	// Ensure wrong alignment (max_align_t+1)
	struct pvl *pvl = pvl_init(pvlbuf+1, main_mem, 1024, 1);
	assert (pvl == NULL);
}

void test_init_alignment() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char main_mem[1024];
	struct pvl *pvl = pvl_init(pvlbuf, main_mem, 1024, 1);
	assert(pvl != NULL);
}

void test_init_zero_marks() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(0)];
	char main_mem[1024];
	struct pvl *pvl = pvl_init(pvlbuf, main_mem, 1024, 0);
	assert(pvl == NULL);
}

void test_init_null_main() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	struct pvl *pvl = pvl_init(pvlbuf, NULL, 1024, 1);
	assert(pvl == NULL);
}

void test_init_non_null_main() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char main_mem[1024];
	struct pvl *pvl = pvl_init(pvlbuf, main_mem, 1024, 1);
	assert(pvl != NULL);
}

void test_init_zero_length() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char main_mem[1024];
	struct pvl *pvl = pvl_init(pvlbuf, main_mem, 0, 1);
	assert(pvl == NULL);
}

void test_init_non_zero_length() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char main_mem[1024];
	struct pvl *pvl = pvl_init(pvlbuf, main_mem, 1024, 1);
	assert(pvl != NULL);
}

void test_init_non_divisible_spans() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char main_mem[1024];
	struct pvl *pvl = pvl_init(pvlbuf, main_mem, 1024, 3);
	assert(pvl == NULL);
}

void test_init_main_mirror_overlap() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 512, 1);
	assert(pvl != NULL);
	assert(pvl_set_mirror(pvl, buffer+256) != 0);
}

void test_init_main_mirror_no_overlap() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 512, 1);
	assert(pvl != NULL);
	assert(pvl_set_mirror(pvl, buffer+512) == 0);
}

void test_init_mirror_main_overlap() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer+256, 512, 1);
	assert(pvl != NULL);
	assert(pvl_set_mirror(pvl, buffer) != 0);
}

void test_init_mirror_main_no_overlap() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer+512, 512, 1);
	assert(pvl != NULL);
	assert(pvl_set_mirror(pvl, buffer) == 0);
}

void test_mark_pvl_null() {
	start_test;
	struct pvl *pvl = NULL;
	int data = 0;
	// pass some garbage
	assert(pvl_mark(pvl, (char*) &data, 28));
}

void test_mark_mark_null() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(pvl_mark(pvl, NULL, 128));
}

void test_mark_zero_length() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(pvl_mark(pvl, buffer, 0));
}

void test_mark_before_main() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer+512, 512, 1);
	assert(pvl_mark(pvl, buffer, 64));
}

void test_mark_mark_main_overlap() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer+512, 512, 1);
	assert(pvl_mark(pvl, buffer+496, 64));
}

void test_mark_main_mark_overlap() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 512, 1);
	assert(pvl_mark(pvl, buffer+496, 64));
}

void test_mark_after_main() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 512, 1);
	assert(pvl_mark(pvl, buffer+768, 64));
}

void test_mark_start_of_main() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(!pvl_mark(pvl, buffer, 64));
}

void test_mark_middle_of_main() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(!pvl_mark(pvl, buffer+512, 64));
}

void test_mark_end_of_main() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(!pvl_mark(pvl, buffer+960, 64));
}

void test_mark_all_of_main() {
	start_test;
	int marks = 8;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, marks);
	assert(!pvl_mark(pvl, buffer, 1024));
}

void test_mark_extra_marks() {
	start_test;
	int marks = 2;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, marks);
	assert(!pvl_mark(pvl, buffer, 10));
	assert(!pvl_mark(pvl, buffer, 20));
	assert(!pvl_mark(pvl, buffer, 30));
}

void test_commit_pvl_null() {
	start_test;
	struct pvl *pvl = NULL;
	// pass some garbage
	assert(pvl_commit(pvl));
}

void test_commit_no_mark() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(!pvl_commit(pvl));
}

void test_commit_single_mark() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(!pvl_mark(pvl, buffer, 128));
	assert(!pvl_commit(pvl));
}

void test_commit_max_marks() {
	start_test;
	size_t marks = 8;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, marks);
	for (size_t i = 0; i < marks; i++) {
		assert(!pvl_mark(pvl, buffer+i, 1));
	}
	assert(!pvl_commit(pvl));
}

void test_commit_over_max_marks() {
	start_test;
	size_t marks = 8;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	for (size_t i = 0; i < marks; i++) {
		assert(!pvl_mark(pvl, buffer+i, 1));
	}
	assert(!pvl_commit(pvl));
}

int noop_read_cb(void *ctx, void *to, size_t length, size_t remaining) {
	(void)(ctx);
	(void)(to);
	(void)(length);
	(void)(remaining);
	return EOF;
}

int noop_write_cb(void *ctx, void *from, size_t length, size_t remaining) {
	(void)(ctx);
	(void)(from);
	(void)(length);
	(void)(remaining);
	return 0;
}

void noop_leak_cb(void *ctx, void *start, size_t length) {
	(void)(ctx);
	(void)(start);
	(void)(length);
}

void test_set_read_cb_null_pvl() {
	start_test;
	struct pvl *pvl = NULL;
	assert(pvl_set_read_cb(pvl, NULL, noop_read_cb) != 0);
}

void test_set_write_cb_null_pvl() {
	start_test;
	struct pvl *pvl = NULL;
	assert(pvl_set_write_cb(pvl, NULL, noop_write_cb) != 0);
}

void test_set_mirror_null_pvl() {
	start_test;
	struct pvl *pvl = NULL;
	char mirror[1024];
	assert(pvl_set_mirror(pvl, mirror) != 0);
}

void test_set_leak_cb_null_pvl() {
	start_test;
	struct pvl *pvl = NULL;
	assert(pvl_set_leak_cb(pvl, NULL, noop_leak_cb) != 0);
}

void test_set_read_cb_twice() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(pvl_set_read_cb(pvl, NULL, noop_read_cb) == 0);
	assert(pvl_set_read_cb(pvl, NULL, noop_read_cb) != 0);
}

void test_set_write_cb_twice() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(pvl_set_write_cb(pvl, NULL, noop_write_cb) == 0);
	assert(pvl_set_write_cb(pvl, NULL, noop_write_cb) != 0);
	// Ensure line coverage
	noop_write_cb(NULL, NULL, 0, 0);
}

void test_set_mirror_twice() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	char mirror[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(pvl_set_mirror(pvl, mirror) == 0);
	assert(pvl_set_mirror(pvl, mirror) != 0);
}

void test_set_leak_cb_twice() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	char mirror[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(pvl_set_mirror(pvl, mirror) == 0);
	assert(pvl_set_leak_cb(pvl, NULL, noop_leak_cb) == 0);
	assert(pvl_set_leak_cb(pvl, NULL, noop_leak_cb) != 0);
	// Ensure line coverage
	noop_leak_cb(NULL, NULL, 0);
}

void test_init_leak_no_mirror() {
	start_test;
	alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
	char buffer[1024];
	struct pvl *pvl = pvl_init(pvlbuf, buffer, 1024, 1);
	assert(pvl != NULL);
	assert(pvl_set_leak_cb(pvl, NULL, noop_leak_cb) != 0);
	// call empty cb, ensure line coverage
	noop_leak_cb(NULL, NULL, 0);
}

typedef struct {
	int	return_int;
	size_t expected_length;
	size_t expected_remaining;
} read_mock;

typedef struct {
	int	return_int;
	size_t expected_length;
	size_t expected_remaining;
} write_mock;

typedef struct {
	struct pvl *expected_pvl;
	void	   *expected_start;
	size_t	 expected_length;
} leak_mock;

#define CTX_BUFFER_SIZE 1024

typedef struct {
	struct pvl *pvl;
	alignas(max_align_t) char pvl_at[CTX_BUFFER_SIZE];
	char main[CTX_BUFFER_SIZE];
	char mirror[CTX_BUFFER_SIZE];

	/*
	 * Test callbacks will assert whether
	 * the expected values and return based
	 * on what's set in their context structs.
	 */
	char iobuf[CTX_BUFFER_SIZE];
	size_t iobuf_pos;

	read_mock	 read_data[10];
	int		   read_pos;

	write_mock	write_data[10];
	int		   write_pos;

	leak_mock	 leak_data[10];
	int		   leak_pos;
} test_ctx;

int read_cb(void *ctx, void *to, size_t length, size_t remaining) {
	test_ctx *t = (test_ctx*) ctx;

	/* printf("\n [read callback] [%d] \n", t->read_pos); */
	read_mock fix = t->read_data[t->read_pos];

	/* printf("   length: %zu expected: %zu\n", length, fix.expected_length); */
	assert(length == fix.expected_length);
	/* printf("remaining: %zu expected: %zu\n", remaining, fix.expected_remaining); */
	assert(remaining == fix.expected_remaining);

	if (to && length) {
		memcpy(to, t->iobuf+t->iobuf_pos, length);
		t->iobuf_pos += length;
	}

	t->read_pos++;
	return fix.return_int;
}

int write_cb(void *ctx, void *from, size_t length, size_t remaining) {
	test_ctx *t = (test_ctx*) ctx;

	/* printf("\n [write callback] [%d] \n", t->write_pos); */
	write_mock fix = t->write_data[t->write_pos];

	/* printf("   length: %zu expected: %zu\n", length, fix.expected_length); */
	assert(length == fix.expected_length);

	/* printf("remaining: %zu expected: %zu\n", remaining, fix.expected_remaining); */
	assert(remaining == fix.expected_remaining);

	memcpy(t->iobuf+t->iobuf_pos, from, length);
	t->iobuf_pos += length;

	t->write_pos++;
	return fix.return_int;
}

void leak_cb(void *ctx, void *start, size_t length) {
	test_ctx *t = (test_ctx*) ctx;

	/* printf("\n	[leak] [%d] \n", t->leak_pos); */
	leak_mock fix = t->leak_data[t->leak_pos];

	/* printf("  start: %p expected: %p\n", start, fix.expected_start); */
	assert(start == fix.expected_start);
	/* printf(" length: %zu expected: %zu\n", length, fix.expected_length); */
	assert(length == fix.expected_length);
	t->leak_pos++;
}

void test_basic_commit() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	ctx.write_data[3].expected_length = pvl_header_size;
	ctx.write_data[3].expected_remaining = pvl_header_size + ((CTX_BUFFER_SIZE/marks_count)*2);
	ctx.write_data[3].return_int = 0;

	ctx.write_data[4].expected_length = pvl_header_size;
	ctx.write_data[4].expected_remaining = (CTX_BUFFER_SIZE/marks_count)*2;
	ctx.write_data[4].return_int = 0;

	ctx.write_data[5].expected_length = (CTX_BUFFER_SIZE/marks_count)*2;
	ctx.write_data[5].expected_remaining = 0;
	ctx.write_data[5].return_int = 0;

	// Write and commit some more data
	memset(ctx.main, 1, 255);
	assert(!pvl_mark(ctx.pvl, ctx.main, 255));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 6);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = 0;

	ctx.read_data[5].expected_length = 0;
	ctx.read_data[5].expected_remaining = pvl_header_size + ((CTX_BUFFER_SIZE/marks_count)*2);
	ctx.read_data[5].return_int = 0;

	ctx.read_data[6].expected_length = pvl_header_size;
	ctx.read_data[6].expected_remaining = ((CTX_BUFFER_SIZE/marks_count)*2);
	ctx.read_data[6].return_int = 0;

	ctx.read_data[7].expected_length = ((CTX_BUFFER_SIZE/marks_count)*2);
	ctx.read_data[7].expected_remaining = 0;
	ctx.read_data[7].return_int = 0;

	ctx.read_data[8].expected_length = pvl_header_size;
	ctx.read_data[8].expected_remaining = 0;
	ctx.read_data[8].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 0);
	assert(ctx.read_pos == 9);

	// Verify it
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		if (i < 255) {
			assert(ctx.main[i] == 1);
		} else {
			assert(ctx.main[i] == 0);
		}
	}
}

void test_basic_commit_mirror() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_mirror(ctx.pvl, ctx.mirror) == 0);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	ctx.write_data[3].expected_length = pvl_header_size;
	ctx.write_data[3].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[3].return_int = 0;

	ctx.write_data[4].expected_length = pvl_header_size;
	ctx.write_data[4].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[4].return_int = 0;

	ctx.write_data[5].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[5].expected_remaining = 0;
	ctx.write_data[5].return_int = 0;

	// Write and commit some more data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 6);

	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		if (i < 64) {
			assert(ctx.mirror[i] == 1);
		} else {
			assert(ctx.mirror[i] == 0);
		}
	}

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);
	memset(ctx.mirror, 0, CTX_BUFFER_SIZE);

	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_mirror(ctx.pvl, ctx.mirror) == 0);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 0);
	assert(ctx.read_pos == 5);

	// Verify it
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		if (i < 64) {
			assert(ctx.main[i] == 1);
		} else {
			assert(ctx.main[i] == 0);
		}
	}

	// Verify it
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		if (i < 64) {
			assert(ctx.mirror[i] == 1);
		} else {
			assert(ctx.mirror[i] == 0);
		}
	}
}

void test_read_failure_01() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	ctx.write_data[3].expected_length = pvl_header_size;
	ctx.write_data[3].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[3].return_int = 0;

	ctx.write_data[4].expected_length = pvl_header_size;
	ctx.write_data[4].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[4].return_int = 0;

	ctx.write_data[5].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[5].expected_remaining = 0;
	ctx.write_data[5].return_int = 0;

	// Write and commit some more data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 6);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 1;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) != 0);
	assert(ctx.read_pos == 3);
}

void test_read_failure_02() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	ctx.write_data[3].expected_length = pvl_header_size;
	ctx.write_data[3].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[3].return_int = 0;

	ctx.write_data[4].expected_length = pvl_header_size;
	ctx.write_data[4].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[4].return_int = 0;

	ctx.write_data[5].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[5].expected_remaining = 0;
	ctx.write_data[5].return_int = 0;

	// Write and commit some more data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 6);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 1;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 1);
	assert(ctx.read_pos == 4);
}

void test_read_partial_failure_01() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	ctx.write_data[3].expected_length = pvl_header_size;
	ctx.write_data[3].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[3].return_int = 0;

	ctx.write_data[4].expected_length = pvl_header_size;
	ctx.write_data[4].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[4].return_int = 0;

	ctx.write_data[5].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[5].expected_remaining = 0;
	ctx.write_data[5].return_int = 0;

	// Write and commit some more data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 6);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 1;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 0);
	assert(ctx.read_pos == 1);
}

void test_read_partial_failure_02() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	ctx.write_data[3].expected_length = pvl_header_size;
	ctx.write_data[3].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[3].return_int = 0;

	ctx.write_data[4].expected_length = pvl_header_size;
	ctx.write_data[4].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[4].return_int = 0;

	ctx.write_data[5].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[5].expected_remaining = 0;
	ctx.write_data[5].return_int = 0;

	// Write and commit some more data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 6);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 1;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 0);
	assert(ctx.read_pos == 2);
}

void test_write_no_marks() {
	start_test;
	size_t marks_count = CTX_BUFFER_SIZE/32;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_mirror(ctx.pvl, ctx.mirror) == 0);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	// Commit with no data
	assert(!pvl_commit(ctx.pvl));
	assert(ctx.write_pos == 0);
}

void test_write_failure_01() {
	start_test;
	size_t marks_count = CTX_BUFFER_SIZE/32;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_mirror(ctx.pvl, ctx.mirror) == 0);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 1;

	// Write and commit some data
	memset(ctx.main, 1, 31);
	assert(!pvl_mark(ctx.pvl, ctx.main, 31));
	assert(pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 1);
}

void test_write_failure_02() {
	start_test;
	size_t marks_count = CTX_BUFFER_SIZE/32;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_mirror(ctx.pvl, ctx.mirror) == 0);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 1;

	// Write and commit some data
	memset(ctx.main, 1, 31);
	assert(!pvl_mark(ctx.pvl, ctx.main, 31));
	assert(pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 2);
}

void test_write_failure_03() {
	start_test;
	size_t marks_count = CTX_BUFFER_SIZE/32;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_mirror(ctx.pvl, ctx.mirror) == 0);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 1;

	// Write and commit some data
	memset(ctx.main, 1, 31);
	assert(!pvl_mark(ctx.pvl, ctx.main, 31));
	assert(pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);
}

void test_invalid_change_header_01() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	// Corrupt the header
	size_t *h_ptr = (size_t *) ctx.iobuf;
	h_ptr[0] = 0;

	// Proper read handlers
	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 0);
	assert(ctx.read_pos == 1);

	// Verify that no changes have been applied
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		assert(ctx.main[i] == 0);
	}
}

void test_invalid_change_header_02() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	// Corrupt the header
	size_t *h_ptr = (size_t *) ctx.iobuf;
	h_ptr[0] = (CTX_BUFFER_SIZE/2)+1;

	// Proper read handlers
	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 0);
	assert(ctx.read_pos == 1);

	// Verify that no changes have been applied
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		assert(ctx.main[i] == 0);
	}
}

void test_invalid_change_header_03() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	// Corrupt the header
	size_t *h_ptr = (size_t *) ctx.iobuf;
	h_ptr[1] = 0;

	// Proper read handlers
	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 0);
	assert(ctx.read_pos == 1);

	// Verify that no changes have been applied
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		assert(ctx.main[i] == 0);
	}
}

void test_invalid_change_header_04() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	// Corrupt the header
	size_t *h_ptr = (size_t *) ctx.iobuf;
	h_ptr[1] = (CTX_BUFFER_SIZE/2)*(sizeof(size_t[2])+1)+1;

	// Proper read handlers
	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 0);
	assert(ctx.read_pos == 1);

	// Verify that no changes have been applied
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		assert(ctx.main[i] == 0);
	}
}

void test_invalid_span_header_01() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	// Corrupt the header
	size_t *h_ptr = (size_t *) ctx.iobuf;
	h_ptr[2] = CTX_BUFFER_SIZE+1;

	// Proper read handlers
	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 1);
	assert(ctx.read_pos == 3);

	// Verify that no changes have been applied
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		assert(ctx.main[i] == 0);
	}
}

void test_invalid_span_header_02() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	// Corrupt the header
	size_t *h_ptr = (size_t *) ctx.iobuf;
	h_ptr[3] = CTX_BUFFER_SIZE+1;

	// Proper read handlers
	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 1);
	assert(ctx.read_pos == 3);

	// Verify that no changes have been applied
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		assert(ctx.main[i] == 0);
	}
}

void test_invalid_span_header_03() {
	start_test;
	size_t marks_count = 8;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 64));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);

	// Prepare for reading the data
	ctx.iobuf_pos = 0;
	memset(ctx.main, 0, CTX_BUFFER_SIZE);

	// Corrupt the header
	size_t *h_ptr = (size_t *) ctx.iobuf;
	size_t tmp = h_ptr[2];
	h_ptr[2] = h_ptr[3];
	h_ptr[3] = tmp;

	// Proper read handlers
	ctx.read_data[0].expected_length = pvl_header_size;
	ctx.read_data[0].expected_remaining = 0;
	ctx.read_data[0].return_int = 0;

	ctx.read_data[1].expected_length = 0;
	ctx.read_data[1].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[1].return_int = 0;

	ctx.read_data[2].expected_length = pvl_header_size;
	ctx.read_data[2].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[2].return_int = 0;

	ctx.read_data[3].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.read_data[3].expected_remaining = 0;
	ctx.read_data[3].return_int = 0;

	ctx.read_data[4].expected_length = pvl_header_size;
	ctx.read_data[4].expected_remaining = 0;
	ctx.read_data[4].return_int = EOF;

	// Create a new pvl to read the data
	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_read_cb(ctx.pvl, &ctx, read_cb) == 1);
	assert(ctx.read_pos == 3);

	// Verify that no changes have been applied
	for (size_t i = 0; i < CTX_BUFFER_SIZE; i++) {
		assert(ctx.main[i] == 0);
	}
}

void test_leak_detected() {
	start_test;
	size_t marks_count = CTX_BUFFER_SIZE/32;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_mirror(ctx.pvl, ctx.mirror) == 0);
	assert(pvl_set_leak_cb(ctx.pvl, &ctx, leak_cb) == 0);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	ctx.leak_data[0].expected_start = ctx.main+32;
	ctx.leak_data[0].expected_length = 32;

	// Write and commit some data
	memset(ctx.main, 1, 64);
	assert(!pvl_mark(ctx.pvl, ctx.main, 31));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);
	assert(ctx.leak_pos == 1);
}

void test_leak_no_leak() {
	start_test;
	size_t marks_count = CTX_BUFFER_SIZE/32;

	test_ctx ctx = {0};

	ctx.pvl = pvl_init(ctx.pvl_at, ctx.main, CTX_BUFFER_SIZE, marks_count);
	assert(ctx.pvl != NULL);
	assert(pvl_set_mirror(ctx.pvl, ctx.mirror) == 0);
	assert(pvl_set_leak_cb(ctx.pvl, &ctx, leak_cb) == 0);
	assert(pvl_set_write_cb(ctx.pvl, &ctx, write_cb) == 0);

	ctx.write_data[0].expected_length = pvl_header_size;
	ctx.write_data[0].expected_remaining = pvl_header_size + (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[0].return_int = 0;

	ctx.write_data[1].expected_length = pvl_header_size;
	ctx.write_data[1].expected_remaining = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[1].return_int = 0;

	ctx.write_data[2].expected_length = (CTX_BUFFER_SIZE/marks_count);
	ctx.write_data[2].expected_remaining = 0;
	ctx.write_data[2].return_int = 0;

	ctx.leak_data[0].expected_start = ctx.main+32;
	ctx.leak_data[0].expected_length = 32;

	// Write and commit some data
	memset(ctx.main, 1, 31);
	assert(!pvl_mark(ctx.pvl, ctx.main, 31));
	assert(!pvl_commit(ctx.pvl));

	assert(ctx.write_pos == 3);
	assert(ctx.leak_pos == 0);
}

void test_bitset_basic() {
	start_test;
	unsigned char buf[4] = {0};
	assert(bitset_test(buf, 0) == 0);
	bitset_set(buf, 0);
	assert(bitset_test(buf, 0) == 1);
	bitset_flip(buf, 0);
	assert(bitset_test(buf, 0) == 0);
	bitset_flip(buf, 0);
	assert(bitset_test(buf, 0) == 1);
}

void test_bitset_range() {
	start_test;
	unsigned char buf[4] = {0};
	size_t bitset_length = 32;
	for (size_t i = 0; i < bitset_length; i++) {
		for (size_t j = 0; j <= i; j++) {
			memset(buf, 0, 4);
			bitset_set_range(buf, j, i);
			for (size_t k = 0; k < bitset_length; k++) {
				if ((k >= j) && (k <= i)) {
					assert(bitset_test(buf, k));
				} else {
					assert(!bitset_test(buf, k));
				}
			}
			bitset_clear_range(buf, j, i);
			for (size_t k = j; k < i; k++) {
				assert(!bitset_test(buf, k));
			}
		}
	}
}

void test_bbt_sizeof_invalid_order() {
	start_test;
	assert(bbt_sizeof(0) == 0);
	assert(bbt_sizeof(sizeof(size_t) * CHAR_BIT) == 0);
}

void test_bbt_init_invalid_order() {
	start_test;
	alignas(max_align_t) unsigned char buf[1024];
	assert(bbt_init(buf, 0) == NULL);
	assert(bbt_init(buf, sizeof(size_t) * CHAR_BIT) == NULL);
	assert(bbt_order(NULL) == 0);
}

void test_bbt_basic() {
	start_test;

	assert(bbt_pos_valid(NULL, 1) == 0);

	size_t order = 3;
	alignas(max_align_t) unsigned char buf[bbt_sizeof(order)];
	struct bbt *bbt = bbt_init(buf, order);
	assert(bbt != NULL);
	assert(bbt_pos_valid(bbt, 256) == 0);
	bbt_pos pos = 0;
	for (size_t i = 0; i < 3; i++) {
		pos = bbt_left_pos_at_depth(bbt, i);
		assert (pos != 0);
		assert (bbt_pos_depth(bbt, pos) == i);
	}
	pos = bbt_left_pos_at_depth(bbt, 0);
	assert(bbt_pos_index(bbt, pos) == 0);
	assert(bbt_pos_sibling(bbt, pos) == 0);
	assert(bbt_pos_parent(bbt, pos) == 0);

	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert((pos = bbt_pos_sibling(bbt, pos)));
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 1);
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 1);
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert((pos = bbt_pos_right_adjacent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 1);
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert((pos = bbt_pos_left_adjacent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 1);
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	bbt_pos_set(bbt, pos);
	assert(bbt_pos_test(bbt, pos) == 1);
	bbt_pos_clear(bbt, pos);
	assert(bbt_pos_test(bbt, pos) == 0);
	bbt_pos_flip(bbt, pos);
	assert(bbt_pos_test(bbt, pos) == 1);
	bbt_pos_flip(bbt, pos);
	assert(bbt_pos_test(bbt, pos) == 0);

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_left_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_left_adjacent(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_right_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_right_adjacent(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_right_adjacent(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert(!bbt_pos_valid(bbt, bbt_pos_parent(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 3);
	assert(!bbt_pos_valid(bbt, bbt_pos_depth(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert(!bbt_pos_valid(bbt, bbt_pos_left_adjacent(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_right_adjacent(bbt, pos)));

	pos = 0; /* invalid pos */
	assert(bbt_pos_left_child(bbt, pos) == 0);
	assert(bbt_pos_right_child(bbt, pos) == 0);
	assert(bbt_pos_left_adjacent(bbt, pos) == 0);
	assert(bbt_pos_right_adjacent(bbt, pos) == 0);
	assert(bbt_pos_sibling(bbt, pos) == 0);
	assert(bbt_pos_parent(bbt, pos) == 0);
	assert(bbt_pos_test(bbt, pos) == 0);
	assert(bbt_pos_depth(bbt, pos) == 0);
	assert(bbt_pos_index(bbt, pos) == 0);
	/* coverage for void functions */
	bbt_pos_set(bbt, pos);
	bbt_pos_clear(bbt, pos);
	bbt_pos_flip(bbt, pos);
	bbt_debug_pos_print(bbt, 1);
	bbt_order(bbt);
}

void test_buddy_init_null() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)+1];
	alignas(max_align_t) unsigned char data_buf[4096+1];
	{
		struct buddy *buddy = buddy_init(NULL, data_buf, 4096);
		assert (buddy == NULL);
	}
	{
		struct buddy *buddy = buddy_init(buddy_buf, NULL, 4096);
		assert (buddy == NULL);
	}
}

void test_buddy_misalignment() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)+1];
	alignas(max_align_t) unsigned char data_buf[4096+1];
	{
		struct buddy *buddy = buddy_init(buddy_buf+1, data_buf, 4096);
		assert (buddy == NULL);
	}
	{
		struct buddy *buddy = buddy_init(buddy_buf, data_buf+1, 4096);
		assert (buddy == NULL);
	}
}

void test_buddy_invalid_datasize() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	{
		assert(buddy_sizeof(0) == 0);
		assert(buddy_sizeof(BUDDY_ALIGN-1) == 0);
	}
	{
		struct buddy *buddy = buddy_init(buddy_buf, data_buf, 0);
		assert (buddy == NULL);
	}
}

void test_buddy_init() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	{
		struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
		assert (buddy != NULL);
	}
}

void test_buddy_init_non_power_of_two_memory() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];

	size_t cutoff = 256;
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096-cutoff);
	assert (buddy != NULL);

	for (size_t i = 0; i < 60; i++) {
		assert(buddy_malloc(buddy, BUDDY_ALIGN) != NULL);
	}
	assert(buddy_malloc(buddy, BUDDY_ALIGN) == NULL);
}

void test_buddy_malloc_null() {
	start_test;
	assert(buddy_malloc(NULL, 1024) == NULL);
}

void test_buddy_malloc_zero() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 0) == NULL);
}

void test_buddy_malloc_larger() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 8192) == NULL);
}

void test_buddy_malloc_basic_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(1024)];
	alignas(max_align_t) unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 1024);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 1024) == NULL);
	buddy_free(buddy, data_buf);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 1024) == NULL);
}

void test_buddy_malloc_basic_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 2048) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == NULL);
	buddy_free(buddy, data_buf);
	buddy_free(buddy, data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == NULL);
}

void test_buddy_malloc_basic_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 1024) == data_buf+1024);
	assert(buddy_malloc(buddy, 1024) == NULL);
	buddy_free(buddy, data_buf+1024);
	buddy_free(buddy, data_buf+2048);
	buddy_free(buddy, data_buf);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 1024) == data_buf+1024);
	assert(buddy_malloc(buddy, 1024) == NULL);
}

void test_buddy_malloc_basic_04() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 64) == data_buf);
	assert(buddy_malloc(buddy, 32) == data_buf+64);
}

void test_buddy_free_coverage() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf+1024, 1024);
	assert (buddy != NULL);
	buddy_free(NULL, NULL);
	buddy_free(NULL, data_buf+1024);
	buddy_free(buddy, NULL);
	buddy_free(buddy, data_buf);
	buddy_free(buddy, data_buf+2048);
}

void test_buddy_demo() {
	size_t arena_size = 65536;
	/* You need space for the metadata and for the arena */
	void *buddy_metadata = malloc(buddy_sizeof(arena_size));
	void *buddy_arena = malloc(arena_size);
	struct buddy *buddy = buddy_init(buddy_metadata, buddy_arena, arena_size);

	/* Allocate using the buddy allocator */
	void *data = buddy_malloc(buddy, 2048);
	/* Free using the buddy allocator */
	buddy_free(buddy, data);

	free(buddy_metadata);
	free(buddy_arena);
}

void test_buddy_demo_embedded() {
	size_t arena_size = 65536;
	/* You need space for arena and builtin metadata */
	void *buddy_arena = malloc(arena_size);
	struct buddy *buddy = buddy_embed(buddy_arena, arena_size);

	/* Allocate using the buddy allocator */
	void *data = buddy_malloc(buddy, 2048);
	/* Free using the buddy allocator */
	buddy_free(buddy, data);

	free(buddy_arena);
}

void test_buddy_calloc() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	memset(data_buf, 1, 4096);
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, sizeof(char), 4096);
	for (size_t i = 0; i < 4096; i++) {
		assert(result[i] == 0);
	}
}

void test_buddy_calloc_no_members() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, 0, 4096);
	assert(result == NULL);
}

void test_buddy_calloc_no_size() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, sizeof(char), 0);
	assert(result == NULL);
}

void test_buddy_calloc_overflow() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	short *result = buddy_calloc(buddy, sizeof(short), SIZE_MAX);
	assert(result == NULL);
}

void test_buddy_realloc_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 0);
	assert(result == NULL);
}

void test_buddy_realloc_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
}

void test_buddy_realloc_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 128);
	assert(result == data_buf);
}

void test_buddy_realloc_04() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 64);
	assert(result == data_buf);
}

void test_buddy_realloc_05() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 256);
	assert(result == data_buf);
}

void test_buddy_realloc_06() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 0);
	assert(result == NULL);
}

void test_buddy_realloc_07() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 1024);
	assert(result == NULL);
}

void test_buddy_realloc_08() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 256) == data_buf);
	void *result = buddy_realloc(buddy, NULL, 256);
	assert(result == data_buf + 256);
	result = buddy_realloc(buddy, result, 512);
	assert(result == NULL);
}

void test_buddy_reallocarray_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, 0, 0);
	assert(result == NULL);
}

void test_buddy_reallocarray_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, sizeof(short), SIZE_MAX);
	assert(result == NULL);
}

void test_buddy_reallocarray_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, sizeof(char), 256);
	assert(result == data_buf);
}

void test_buddy_embedded_not_enough_memory() {
	start_test;
	alignas(max_align_t) unsigned char buf[4];
	struct buddy *buddy = buddy_embed(buf, 4);
	assert(buddy == NULL);
}

void test_buddy_embedded_null() {
	start_test;
	struct buddy *buddy = buddy_embed(NULL, 4096);
	assert(buddy == NULL);
}

void test_buddy_embedded_01() {
	start_test;
	alignas(max_align_t) unsigned char buf[4096];
	struct buddy *buddy = buddy_embed(buf, 4096);
	assert(buddy != NULL);
}

void test_buddy_embedded_malloc_01() {
	start_test;
	alignas(max_align_t) unsigned char buf[4096];
	struct buddy *buddy = buddy_embed(buf, 4096);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 2048) == buf);
	assert(buddy_malloc(buddy, 2048) == NULL);
	buddy_free(buddy, buf);
	assert(buddy_malloc(buddy, 2048) == buf);
	assert(buddy_malloc(buddy, 2048) == NULL);
}

void test_buddy_mixed_use_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *addr[8] = {0};
	for (size_t i = 0; i < 8; i++) {
		addr[i] = buddy_malloc(buddy, 64);
		assert(addr[i] != NULL);
	}
	for (size_t i = 0; i < 8; i++) {
		if (i%2 == 0) {
			buddy_free(buddy, addr[i]);
		}
	}
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) == NULL);
}

void test_buddy_mixed_use_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *addr[8] = {0};
	for (size_t i = 0; i < 8; i++) {
		addr[i] = buddy_malloc(buddy, 64);
		assert(addr[i] != NULL);
	}
	for (size_t i = 0; i < 8; i++) {
		buddy_free(buddy, addr[i]);
	}
	assert(buddy_malloc(buddy, 256) != NULL);
	assert(buddy_malloc(buddy, 128) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) == NULL);
}

void test_buddy_mixed_use_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *addr[4] = {0};
	for (size_t i = 0; i < 4; i++) {
		addr[i] = buddy_malloc(buddy, 128);
		assert(addr[i] != NULL);
	}
	for (size_t i = 0; i < 4; i++) {
		buddy_free(buddy, addr[i]);
	}
	assert(buddy_malloc(buddy, 256) != NULL);
	assert(buddy_malloc(buddy, 256) != NULL);
	assert(buddy_malloc(buddy, 256) == NULL);
}

void test_buddy_mixed_sizes_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy_malloc(buddy, 0) == NULL);
	for(size_t i = 1; i <= 64; i++) {
		printf("%zu\n", i); fflush(stdout);
		assert(buddy_malloc(buddy, i) == data_buf+((i-1)*64));
	}
	assert(buddy_malloc(buddy, 1) == NULL);
}

void test_buddy_tree_sizeof() {
	start_test;
	assert(buddy_tree_sizeof(0) == 0);
	assert(buddy_tree_sizeof(1) == 2);
	assert(buddy_tree_sizeof(2) == 2);
	assert(buddy_tree_sizeof(3) == 3);
	assert(buddy_tree_sizeof(4) == 4);
	assert(buddy_tree_sizeof(5) == 8);
	assert(buddy_tree_sizeof(6) == 14);
	assert(buddy_tree_sizeof(7) == 27);
	assert(buddy_tree_sizeof(8) == 53);
	assert(buddy_tree_sizeof(9) == 106);
	assert(buddy_tree_sizeof(10) == 210);
	assert(buddy_tree_sizeof(11) == 419);
	assert(buddy_tree_sizeof(12) == 837);
	assert(buddy_tree_sizeof(13) == 1673);
	assert(buddy_tree_sizeof(14) == 3345);
	assert(buddy_tree_sizeof(15) == 6689);
	assert(buddy_tree_sizeof(16) == 13377);
	assert(buddy_tree_sizeof(17) == 26753);
	assert(buddy_tree_sizeof(18) == 53506);
	assert(buddy_tree_sizeof(19) == 107011);
	assert(buddy_tree_sizeof(20) == 214021);
	assert(buddy_tree_sizeof(21) == 428041);
	assert(buddy_tree_sizeof(22) == 856081);
	assert(buddy_tree_sizeof(23) == 1712161);
	assert(buddy_tree_sizeof(24) == 3424321);
	assert(buddy_tree_sizeof(25) == 6848641);
	assert(buddy_tree_sizeof(26) == 13697281);
	assert(buddy_tree_sizeof(27) == 27394561);
	assert(buddy_tree_sizeof(28) == 54789121);
	assert(buddy_tree_sizeof(29) == 109578241);
	assert(buddy_tree_sizeof(30) == 219156481);
	assert(buddy_tree_sizeof(31) == 438312961);
	assert(buddy_tree_sizeof(32) == 876625921);
	assert(buddy_tree_sizeof(33) == 1753251841);
	assert(buddy_tree_sizeof(34) == 3506503682); /* 3 GB */
	assert(buddy_tree_sizeof(35) == 7013007363);
	assert(buddy_tree_sizeof(36) == 14026014725);
	assert(buddy_tree_sizeof(37) == 28052029449);
	assert(buddy_tree_sizeof(38) == 56104058897);
	assert(buddy_tree_sizeof(39) == 112208117793);
	assert(buddy_tree_sizeof(40) == 224416235585);
	assert(buddy_tree_sizeof(41) == 448832471169);
	assert(buddy_tree_sizeof(42) == 897664942337);
	assert(buddy_tree_sizeof(43) == 1795329884673);
	assert(buddy_tree_sizeof(44) == 3590659769345);
	assert(buddy_tree_sizeof(45) == 7181319538689);
	assert(buddy_tree_sizeof(46) == 14362639077377);
	assert(buddy_tree_sizeof(47) == 28725278154753);
	assert(buddy_tree_sizeof(48) == 57450556309505); /* 52 TB .. */
}

void test_buddy_tree_init() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	assert(buddy_tree_init(NULL, 8) == NULL);
	assert(buddy_tree_init(buddy_tree_buf, 0) == NULL);
	assert(buddy_tree_init(buddy_tree_buf, 8) != NULL);
}

void test_buddy_tree_valid() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_valid(NULL, 1) == 0);
	assert(buddy_tree_valid(t, 0) == 0);
	assert(buddy_tree_valid(t, 256) == 0);
	assert(buddy_tree_valid(t, 1) == 1);
	assert(buddy_tree_valid(t, 255) == 1);
}

void test_buddy_tree_order() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_order(NULL) == 0);
	assert(buddy_tree_order(t) == 8);
}

void test_buddy_tree_root() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_root(NULL) == 0);
	assert(buddy_tree_root(t) == 1);
}

void test_buddy_tree_depth() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_depth(t, 0) == 0);
	assert(buddy_tree_depth(t, 1) == 1);
}

void test_buddy_tree_left_child() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	pos = buddy_tree_left_child(t, pos);
	assert(buddy_tree_depth(t, pos) == 2);
	pos = buddy_tree_left_child(t, pos);
	assert(buddy_tree_valid(t, pos) == 0);
	pos = buddy_tree_left_child(t, pos);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_right_child() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	pos = buddy_tree_right_child(t, pos);
	assert(buddy_tree_depth(t, pos) == 2);
	pos = buddy_tree_right_child(t, pos);
	assert(buddy_tree_valid(t, pos) == 0);
	pos = buddy_tree_right_child(t, pos);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_parent() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_parent(t, pos) == 0);
	assert(buddy_tree_parent(t, 0) == 0);
	assert(buddy_tree_parent(t, buddy_tree_left_child(t, pos)) == pos);
	assert(buddy_tree_parent(t, buddy_tree_right_child(t, pos)) == pos);
}

void test_buddy_tree_sibling() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_sibling(t, pos) == 0);
	assert(buddy_tree_sibling(t, 0) == 0);
	assert(buddy_tree_sibling(t, buddy_tree_left_child(t, pos)) == buddy_tree_right_child(t, pos));
	assert(buddy_tree_sibling(t, buddy_tree_right_child(t, pos)) == buddy_tree_left_child(t, pos));
}

void test_buddy_tree_left_adjacent() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_left_adjacent(t, 0) == 0);
	assert(buddy_tree_left_adjacent(t, pos) == 0);
	assert(buddy_tree_left_adjacent(t, buddy_tree_left_child(t, pos)) == 0);
	assert(buddy_tree_left_adjacent(t, buddy_tree_right_child(t, pos)) == buddy_tree_left_child(t, pos));
}

void test_buddy_tree_right_adjacent() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_right_adjacent(t, 0) == 0);
	assert(buddy_tree_right_adjacent(t, pos) == 0);
	assert(buddy_tree_right_adjacent(t, buddy_tree_right_child(t, pos)) == 0);
	assert(buddy_tree_right_adjacent(t, buddy_tree_left_child(t, pos)) == buddy_tree_right_child(t, pos));
}

void test_buddy_tree_index() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_index(t, 0) == 0);
	assert(buddy_tree_index(t, pos) == 0);
	assert(buddy_tree_index(t, buddy_tree_left_child(t, pos)) == 0);
	assert(buddy_tree_index(t, buddy_tree_right_child(t, pos)) == 1);
}

void test_buddy_tree_mark_status_release_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);

	assert(buddy_tree_status(t, 0) == 0);
	buddy_tree_mark(t, 0); /* coverage */
	buddy_tree_release(t, 0); /* coverage */

	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 1);
	buddy_tree_release(t, pos);
	assert(buddy_tree_status(t, pos) == 0);
}

void test_buddy_tree_mark_status_release_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 2);
}

void test_buddy_tree_mark_status_release_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 3);
}

void test_buddy_tree_mark_status_release_04() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 4);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 4);
}

void test_buddy_tree_duplicate_mark() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	buddy_tree_pos pos = buddy_tree_root(t);
	buddy_tree_mark(t, pos);
	buddy_tree_mark(t, pos);
}

void test_buddy_tree_duplicate_free() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	buddy_tree_pos pos = buddy_tree_root(t);
	buddy_tree_release(t, pos);
}

void test_buddy_tree_propagation_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	buddy_tree_pos left = buddy_tree_left_child(t, pos);
	assert(buddy_tree_status(t, left) == 0);
	buddy_tree_mark(t, left);
	assert(buddy_tree_status(t, left) == 1);
	assert(buddy_tree_status(t, pos) == 1);
}

void test_buddy_tree_propagation_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_pos pos = buddy_tree_root(t);
	buddy_tree_pos left = buddy_tree_left_child(t, buddy_tree_left_child(t, pos));
	buddy_tree_mark(t, left);
	assert(buddy_tree_status(t, left) == 1);
	assert(buddy_tree_status(t, pos) == 1);
}

void test_buddy_tree_find_free_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_pos pos = buddy_tree_find_free(t, 0);
	assert(buddy_tree_valid(t, pos) == 0);
	pos = buddy_tree_find_free(t, 4);
	assert(buddy_tree_valid(t, pos) == 0);
	pos = buddy_tree_find_free(NULL, 1);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_find_free_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_pos pos = buddy_tree_find_free(t, 1);
	assert(buddy_tree_valid(t, pos) == 1);
	pos = buddy_tree_find_free(t, 2);
	assert(buddy_tree_valid(t, pos) == 1);

	buddy_tree_debug(t, buddy_tree_root(t));printf("\n");
	buddy_tree_mark(t, pos);
	buddy_tree_debug(t, buddy_tree_root(t));printf("\n");

	pos = buddy_tree_find_free(t, 2);
	assert(buddy_tree_valid(t, pos) == 1);

	buddy_tree_mark(t, pos);
	buddy_tree_debug(t, buddy_tree_root(t));printf("\n");

	pos = buddy_tree_find_free(t, 2);
	assert(buddy_tree_valid(t, pos) == 0);
}

int main() {
	{
		test_init_misalignment();
		test_init_alignment();
		test_init_zero_marks();
		test_init_null_main();
		test_init_non_null_main();
		test_init_zero_length();
		test_init_non_zero_length();
		test_init_non_divisible_spans();
		test_init_main_mirror_overlap();
		test_init_main_mirror_no_overlap();
		test_init_mirror_main_overlap();
		test_init_mirror_main_no_overlap();

		test_mark_pvl_null();
		test_mark_mark_null();
		test_mark_zero_length();
		test_mark_before_main();
		test_mark_mark_main_overlap();
		test_mark_main_mark_overlap();
		test_mark_after_main();
		test_mark_start_of_main();
		test_mark_middle_of_main();
		test_mark_end_of_main();
		test_mark_all_of_main();
		test_mark_extra_marks();

		test_commit_pvl_null();
		test_commit_no_mark();
		test_commit_single_mark();
		test_commit_max_marks();
		test_commit_over_max_marks();

		test_set_read_cb_null_pvl();
		test_set_write_cb_null_pvl();
		test_set_mirror_null_pvl();
		test_set_leak_cb_null_pvl();

		test_set_read_cb_twice();
		test_set_write_cb_twice();
		test_set_mirror_twice();
		test_set_leak_cb_twice();

		test_init_leak_no_mirror();
	}

	{
		test_basic_commit();
		test_basic_commit_mirror();

		test_read_failure_01();
		test_read_failure_02();

		test_read_partial_failure_01();
		test_read_partial_failure_02();

		test_write_no_marks();

		test_write_failure_01();
		test_write_failure_02();
		test_write_failure_03();

		test_invalid_change_header_01();
		test_invalid_change_header_02();
		test_invalid_change_header_03();
		test_invalid_change_header_04();

		test_invalid_span_header_01();
		test_invalid_span_header_02();
		test_invalid_span_header_03();
	}

	{
		test_leak_detected();
		test_leak_no_leak();
	}

	{
		test_bitset_basic();
		test_bitset_range();
	}

	{
		test_bbt_sizeof_invalid_order();
		test_bbt_init_invalid_order();

		test_bbt_basic();
	}

	{
		test_buddy_init_null();
		test_buddy_misalignment();
		test_buddy_invalid_datasize();

		test_buddy_init();
		test_buddy_init_non_power_of_two_memory();

		test_buddy_malloc_null();
		test_buddy_malloc_zero();
		test_buddy_malloc_larger();

		test_buddy_malloc_basic_01();
		test_buddy_malloc_basic_02();
		test_buddy_malloc_basic_03();
		test_buddy_malloc_basic_04();

		test_buddy_free_coverage();

		test_buddy_demo();
		test_buddy_demo_embedded();

		test_buddy_calloc();
		test_buddy_calloc_no_members();
		test_buddy_calloc_no_size();
		test_buddy_calloc_overflow();

		test_buddy_realloc_01();
		test_buddy_realloc_02();
		test_buddy_realloc_03();
		test_buddy_realloc_04();
		test_buddy_realloc_05();
		test_buddy_realloc_06();
		test_buddy_realloc_07();
		test_buddy_realloc_08();

		test_buddy_reallocarray_01();
		test_buddy_reallocarray_02();
		test_buddy_reallocarray_03();

		test_buddy_embedded_not_enough_memory();
		test_buddy_embedded_null();
		test_buddy_embedded_01();
		test_buddy_embedded_malloc_01();

		test_buddy_mixed_use_01();
		test_buddy_mixed_use_02();
		test_buddy_mixed_use_03();

		test_buddy_mixed_sizes_01();
	}

	{
		test_buddy_tree_sizeof();
		test_buddy_tree_init();
		test_buddy_tree_valid();
		test_buddy_tree_order();
		test_buddy_tree_root();
		test_buddy_tree_depth();
		test_buddy_tree_left_child();
		test_buddy_tree_right_child();
		test_buddy_tree_parent();
		test_buddy_tree_sibling();
		test_buddy_tree_left_adjacent();
		test_buddy_tree_right_adjacent();
		test_buddy_tree_index();
		test_buddy_tree_mark_status_release_01();
		test_buddy_tree_mark_status_release_02();
		test_buddy_tree_mark_status_release_03();
		test_buddy_tree_mark_status_release_04();
		test_buddy_tree_duplicate_mark();
		test_buddy_tree_duplicate_free();
		test_buddy_tree_propagation_01();
		test_buddy_tree_propagation_02();
		test_buddy_tree_find_free_01();
		test_buddy_tree_find_free_02();
	}
}
