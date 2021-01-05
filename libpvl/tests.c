#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "pvl.h"

#define pvl_header_size (2*sizeof(size_t))

void test_init_misalignment() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)+1];
    char main_mem[1024];
    // Ensure wrong alignment (max_align_t+1)
    struct pvl *pvl = pvl_init(pvlbuf+1, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert (pvl == NULL);
}

void test_init_alignment() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_zero_marks() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(0)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 0, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_null_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    struct pvl *pvl = pvl_init(pvlbuf, 1, NULL, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_non_null_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_zero_length() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, main_mem, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_non_zero_length() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_non_divisible_spans() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 3, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_main_mirror_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, buffer+256, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_main_mirror_no_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, buffer+512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_mirror_main_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+256, 512, buffer, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_mirror_main_no_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+512, 512, buffer, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_leak_no_mirror_leak_cb(void *ctx, void *start, size_t length) {
    (void)(ctx);
    (void)(start);
    (void)(length);
}

void test_init_leak_no_mirror() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL,
        NULL, test_init_leak_no_mirror_leak_cb);
    assert(pvl == NULL);
    // call empty cb, ensure line coverage
    test_init_leak_no_mirror_leak_cb(NULL, NULL, 0);
}

void test_mark_pvl_null() {
    struct pvl *pvl = NULL;
    int data = 0;
    // pass some garbage
    assert(pvl_mark(pvl, (char*) &data, 28));
}

void test_mark_mark_null() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, NULL, 128));
}

void test_mark_zero_length() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer, 0));
}

void test_mark_before_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+512, 512, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer, 64));
}

void test_mark_mark_main_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+512, 512, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer+496, 64));
}

void test_mark_main_mark_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer+496, 64));
}

void test_mark_after_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer+768, 64));
}

void test_mark_start_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer, 64));
}

void test_mark_middle_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer+512, 64));
}

void test_mark_end_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer+960, 64));
}

void test_mark_all_of_main() {
    int marks = 8;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer, 1024));
}

void test_mark_extra_marks() {
    int marks = 2;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer, 10));
    assert(!pvl_mark(pvl, buffer, 20));
    assert(!pvl_mark(pvl, buffer, 30));
}

void test_commit_pvl_null() {
    struct pvl *pvl = NULL;
    // pass some garbage
    assert(pvl_commit(pvl));
}

void test_commit_no_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_commit(pvl));
}

void test_commit_single_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer, 128));
    assert(!pvl_commit(pvl));
}

void test_commit_max_marks() {
    size_t marks = 8;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    for (size_t i = 0; i < marks; i++) {
        assert(!pvl_mark(pvl, buffer+i, 1));
    }
    assert(!pvl_commit(pvl));
}

void test_commit_over_max_marks() {
    size_t marks = 8;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    for (size_t i = 0; i < marks; i++) {
        assert(!pvl_mark(pvl, buffer+i, 1));
    }
    assert(!pvl_commit(pvl));
}


/*
 * Define the guard to test implementation details
 */
#define WARNING_DO_NOT_INCLUDE_PLV_C
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

    printf("\n [read callback] [%d] \n", t->read_pos);
    read_mock fix = t->read_data[t->read_pos];

    printf("   length: %zu expected: %zu\n", length, fix.expected_length);
    assert(length == fix.expected_length);
    printf("remaining: %zu expected: %zu\n", remaining, fix.expected_remaining);
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

    printf("\n [write callback] [%d] \n", t->write_pos);
    write_mock fix = t->write_data[t->write_pos];

    printf("   length: %zu expected: %zu\n", length, fix.expected_length);
    assert(length == fix.expected_length);

    printf("remaining: %zu expected: %zu\n", remaining, fix.expected_remaining);
    assert(remaining == fix.expected_remaining);

    memcpy(t->iobuf+t->iobuf_pos, from, length);
    t->iobuf_pos += length;

    t->write_pos++;
    return fix.return_int;
}

void leak_cb(void *ctx, void *start, size_t length) {
    test_ctx *t = (test_ctx*) ctx;

    printf("\n    [leak] [%d] \n", t->leak_pos);
    leak_mock fix = t->leak_data[t->leak_pos];

    printf("  start: %p expected: %p\n", start, fix.expected_start);
    assert(start == fix.expected_start);
    printf(" length: %zu expected: %zu\n", length, fix.expected_length);
    assert(length == fix.expected_length);
    t->leak_pos++;
    return;
}

void test_basic_commit() {
    printf("\n[test_basic_commit]\n");
    size_t marks_count = 8;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, &ctx, read_cb, NULL, NULL, NULL, NULL);
	assert(ctx.pvl != NULL);
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
    printf("\n[test_basic_commit_mirror]\n");
    size_t marks_count = 8;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, ctx.mirror, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, ctx.mirror, &ctx, read_cb, NULL, NULL, NULL, NULL);
	assert(ctx.pvl != NULL);
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
    printf("\n[test_read_failure_01]\n");
    size_t marks_count = 8;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, &ctx, read_cb, NULL, NULL, NULL, NULL);
    assert(ctx.pvl == NULL);
    assert(ctx.read_pos == 3);
}

void test_read_failure_02() {
    printf("\n[test_read_failure_02]\n");
    size_t marks_count = 8;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, &ctx, read_cb, NULL, NULL, NULL, NULL);
    assert(ctx.pvl == NULL);
    assert(ctx.read_pos == 4);
}

void test_read_partial_failure_01() {
    printf("\n[test_read_partial_failure_01]\n");
    size_t marks_count = 8;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, &ctx, read_cb, NULL, NULL, NULL, NULL);
    assert(ctx.pvl != NULL);
    assert(ctx.read_pos == 1);
}

void test_read_partial_failure_02() {
    printf("\n[test_read_partial_failure_02]\n");
    size_t marks_count = 8;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, NULL, &ctx, read_cb, NULL, NULL, NULL, NULL);
    assert(ctx.pvl != NULL);
    assert(ctx.read_pos == 2);
}

void test_write_no_marks() {
    printf("\n[test_write_no_marks]\n");
    size_t marks_count = CTX_BUFFER_SIZE/32;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, ctx.mirror, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

    // Commit with no data
    assert(!pvl_commit(ctx.pvl));
    assert(ctx.write_pos == 0);
}

void test_write_failure_01() {
    printf("\n[test_write_failure_01]\n");
    size_t marks_count = CTX_BUFFER_SIZE/32;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, ctx.mirror, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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
    printf("\n[test_write_failure_02]\n");
    size_t marks_count = CTX_BUFFER_SIZE/32;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, ctx.mirror, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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
    printf("\n[test_write_failure_03]\n");
    size_t marks_count = CTX_BUFFER_SIZE/32;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, ctx.mirror, NULL, NULL, &ctx, write_cb, NULL, NULL);
    assert(ctx.pvl != NULL);

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

void test_leak_detected() {
    printf("\n[test_leak_detected]\n");
    size_t marks_count = CTX_BUFFER_SIZE/32;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, ctx.mirror, NULL, NULL, &ctx, write_cb, &ctx, leak_cb);
    assert(ctx.pvl != NULL);

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
    printf("\n[test_leak_no_leak]\n");
    size_t marks_count = CTX_BUFFER_SIZE/32;

    test_ctx ctx = {0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.main, CTX_BUFFER_SIZE, ctx.mirror, NULL, NULL, &ctx, write_cb, &ctx, leak_cb);
    assert(ctx.pvl != NULL);

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
        test_init_leak_no_mirror();

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
    }

    {
        test_leak_detected();
        test_leak_no_leak();
    }
}
