/*
 * Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbm.h"
#include "bbt.h"

#include "pvl.h"

#define pvl_header_size (2*sizeof(size_t))
#define start_test printf("Running test: %s in %s:%d\n", __func__, __FILE__, __LINE__);

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

/*
 * Define the guard to test implementation details
 */
#ifndef WARNING_DO_NOT_INCLUDE_PLV_C
#define WARNING_DO_NOT_INCLUDE_PLV_C
#endif
#include "pvl.c"

typedef struct {
    int    return_int;
    size_t expected_length;
    size_t expected_remaining;
} read_mock;

typedef struct {
    int    return_int;
    size_t expected_length;
    size_t expected_remaining;
} write_mock;

typedef struct {
    struct pvl *expected_pvl;
    void       *expected_start;
    size_t     expected_length;
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

    read_mock     read_data[10];
    int           read_pos;

    write_mock    write_data[10];
    int           write_pos;

    leak_mock     leak_data[10];
    int           leak_pos;
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

    /* printf("\n    [leak] [%d] \n", t->leak_pos); */
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

void test_bbt_init_misalignment() {
	start_test;
    alignas(max_align_t) unsigned char buf[bbt_sizeof(1)+1];
    // Ensure wrong alignment (max_align_t+1)
    struct bbt *bbt = bbt_init(buf+1, 1);
    assert (bbt == NULL);
    assert(bbt_pos_valid(bbt, 1) == 0);
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
	size_t bbt_order = 3;
	alignas(max_align_t) unsigned char buf[bbt_sizeof(bbt_order)];
	struct bbt *bbt = bbt_init(buf, bbt_order);
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
}

void test_bbm_init_null() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)+1];
    alignas(max_align_t) unsigned char data_buf[4096+1];
    {
		struct bbm *bbm = bbm_init(NULL, data_buf, 4096);
		assert (bbm == NULL);
	}
    {
		struct bbm *bbm = bbm_init(bbm_buf, NULL, 4096);
		assert (bbm == NULL);
	}
}

void test_bbm_misalignment() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)+1];
    alignas(max_align_t) unsigned char data_buf[4096+1];
    {
		struct bbm *bbm = bbm_init(bbm_buf+1, data_buf, 4096);
		assert (bbm == NULL);
	}
    {
		struct bbm *bbm = bbm_init(bbm_buf, data_buf+1, 4096);
		assert (bbm == NULL);
	}
}

void test_bbm_invalid_datasize() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
    {
		assert(bbm_sizeof(0) == 0);
		assert(bbm_sizeof(BBM_ALIGN-1) == 0);
		assert(bbm_sizeof(BBM_ALIGN+1) == 0);
	}
    {
		struct bbm *bbm = bbm_init(bbm_buf, data_buf, 0);
		assert (bbm == NULL);
	}
}

void test_bbm_init() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
    {
		struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
		assert (bbm != NULL);
	}
}

void test_bbm_malloc_null() {
	start_test;
	assert(bbm_malloc(NULL, 1024) == NULL);
}

void test_bbm_malloc_zero() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 0) == NULL);
}

void test_bbm_malloc_larger() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 8192) == NULL);
}

void test_bbm_malloc_basic_01() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(1024)];
    alignas(max_align_t) unsigned char data_buf[1024];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 1024);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 1024) == data_buf);
	assert(bbm_malloc(bbm, 1024) == NULL);
	bbm_free(bbm, data_buf);
	assert(bbm_malloc(bbm, 1024) == data_buf);
	assert(bbm_malloc(bbm, 1024) == NULL);
}

void test_bbm_malloc_basic_02() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 2048) == data_buf);
	assert(bbm_malloc(bbm, 2048) == data_buf+2048);
	assert(bbm_malloc(bbm, 2048) == NULL);
	bbm_free(bbm, data_buf);
	bbm_free(bbm, data_buf+2048);
	assert(bbm_malloc(bbm, 2048) == data_buf);
	assert(bbm_malloc(bbm, 2048) == data_buf+2048);
	assert(bbm_malloc(bbm, 2048) == NULL);
}

void test_bbm_malloc_basic_03() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 1024) == data_buf);
	assert(bbm_malloc(bbm, 2048) == data_buf+2048);
	assert(bbm_malloc(bbm, 1024) == data_buf+1024);
	assert(bbm_malloc(bbm, 1024) == NULL);
	bbm_free(bbm, data_buf+1024);
	bbm_free(bbm, data_buf+2048);
	bbm_free(bbm, data_buf);
	assert(bbm_malloc(bbm, 1024) == data_buf);
	assert(bbm_malloc(bbm, 2048) == data_buf+2048);
	assert(bbm_malloc(bbm, 1024) == data_buf+1024);
	assert(bbm_malloc(bbm, 1024) == NULL);
}

void test_bbm_malloc_basic_04() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 64) == data_buf);
	assert(bbm_malloc(bbm, 32) == data_buf+64);
}

void test_bbm_free_coverage() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf+1024, 1024);
	assert (bbm != NULL);
	bbm_debug_print(bbm);
	bbm_free(NULL, NULL);
	bbm_free(NULL, data_buf+1024);
	bbm_free(bbm, NULL);
	bbm_free(bbm, data_buf);
	bbm_free(bbm, data_buf+2048);
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
		test_bbt_init_misalignment();

		test_bbt_sizeof_invalid_order();
		test_bbt_init_invalid_order();

		test_bbt_basic();
	}

	{
		test_bbm_init_null();
		test_bbm_misalignment();
		test_bbm_invalid_datasize();

		test_bbm_init();

		test_bbm_malloc_null();
		test_bbm_malloc_zero();
		test_bbm_malloc_larger();

		test_bbm_malloc_basic_01();
		test_bbm_malloc_basic_02();
		test_bbm_malloc_basic_03();
		test_bbm_malloc_basic_04();

		test_bbm_free_coverage();
	}
}
