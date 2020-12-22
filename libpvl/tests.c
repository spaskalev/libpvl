#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "pvl.h"

void test_init_misalignment() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)+1];
    char main_mem[1024];
    // Ensure wrong alignment (max_align_t+1)
    struct pvl *pvl = pvl_init(pvlbuf+1, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert (pvl == NULL);
}

void test_init_alignment() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_zero_marks() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(0)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 0, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_null_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    struct pvl *pvl = pvl_init(pvlbuf, 1, NULL, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_non_null_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_zero_legnth() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, main_mem, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_non_zero_legnth() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_main_mirror_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, buffer+256, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_main_mirror_no_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, buffer+512, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_mirror_main_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+256, 512, buffer, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

void test_init_mirror_main_no_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+512, 512, buffer, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_leak_no_mirror_leak_cb(struct pvl *pvl, void *start, size_t length) {
    (void)(pvl);
    (void)(start);
    (void)(length);
}

void test_init_leak_no_mirror() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL,
        test_init_leak_no_mirror_leak_cb);
    assert(pvl == NULL);
    // call empty cb, ensure line coverage
    test_init_leak_no_mirror_leak_cb(NULL, NULL, 0, 0);
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
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, NULL, 128));
}

void test_mark_zero_length() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer, 0));
}

void test_mark_before_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+512, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer, 64));
}

void test_mark_mark_main_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+512, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer+496, 64));
}

void test_mark_main_mark_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer+496, 64));
}

void test_mark_after_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer+768, 64));
}

void test_mark_start_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer, 64));
}

void test_mark_middle_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer+512, 64));
}

void test_mark_end_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer+960, 64));
}

void test_mark_all_of_main() {
    int marks = 10;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer, 1024));
}

void test_mark_extra_marks() {
    int marks = 2;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
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
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_commit(pvl));
}

void test_commit_single_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_mark(pvl, buffer, 128));
    assert(!pvl_commit(pvl));
}

void test_commit_max_marks() {
    size_t marks = 10;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    for (size_t i = 0; i < marks; i++) {
        assert(!pvl_mark(pvl, buffer+i, 1));
    }
    assert(!pvl_commit(pvl));
}

void test_commit_over_max_marks() {
    size_t marks = 10;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
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
    FILE   *return_file;

    struct pvl *expected_pvl;
    int        expected_initial;
    FILE       *expected_up_to_src;
    long       expected_up_to_pos;
} pre_load_ctx;

typedef struct {
    int return_int;

    struct pvl *expected_pvl;
    FILE       *expected_file;
    int        expected_failed;
    long       expected_last_good;
} post_load_ctx;

typedef struct {
    FILE *return_file;

    struct pvl *expected_pvl;
    int        expected_full;
    size_t     expected_length;
} pre_save_ctx;

typedef struct {
    int return_int;

    struct pvl *expected_pvl;
    int        expected_full;
    size_t     expected_length;
    FILE       *expected_file;
    int        expected_failed;
} post_save_ctx;

typedef struct {
    struct pvl *expected_pvl;
    void       *expected_start;
    size_t     expected_length;
} leak_ctx;

typedef struct {
    struct pvl *pvl;
    alignas(max_align_t) char pvl_at[1024];
    alignas(max_align_t) char buf[1024];
    alignas(max_align_t) char mirror[1024];

    char   *dyn_buf;
    size_t dyn_len;
    FILE   *dyn_file;

    /*
     * Test callbacks will assert whether
     * the expected values and return based
     * on what's set in their context structs.
     */
    pre_load_ctx  pre_load[10];
    int           pre_load_pos;

    post_load_ctx post_load[10];
    int           post_load_pos;

    pre_save_ctx  pre_save[10];
    int           pre_save_pos;

    post_save_ctx post_save[10];
    int           post_save_pos;

    leak_ctx      leak[10];
    int           leak_pos;
} test_ctx;

// Shared global
test_ctx ctx;

FILE *pre_load(struct pvl *pvl, int initial, FILE *up_to_src, long up_to_pos) {
    printf("\n [pre_load] [%d] \n", ctx.pre_load_pos);
    pre_load_ctx fix = ctx.pre_load[ctx.pre_load_pos];
    printf("      pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("  initial: %d expected: %d\n", initial, fix.expected_initial);
    assert(initial == fix.expected_initial);
    printf("up_to_src: %p expected: %p\n", (void*) up_to_src, (void*) fix.expected_up_to_src);
    assert(up_to_src == fix.expected_up_to_src);
    printf("up_to_pos: %ld expected: %ld\n", up_to_pos, fix.expected_up_to_pos);
    assert(up_to_pos == fix.expected_up_to_pos);
    ctx.pre_load_pos++;
    return fix.return_file;
}

int post_load(struct pvl *pvl, FILE *file, int failed, long last_good) {
    printf("\n[post_load] [%d] \n", ctx.post_load_pos);
    post_load_ctx fix = ctx.post_load[ctx.post_load_pos];
    printf("      pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("     file: %p expected: %p\n", (void*) file, (void*) fix.expected_file);
    assert(file == fix.expected_file);
    printf("   failed: %d expected: %d\n", failed, fix.expected_failed);
    assert(failed == fix.expected_failed);
    printf("last_good: %ld expected: %ld\n", last_good, fix.expected_last_good);
    assert(last_good == fix.expected_last_good);
    ctx.post_load_pos++;
    return fix.return_int;
}

FILE *pre_save(struct pvl *pvl, int full, size_t length) {
    printf("\n [pre_save] [%d] \n", ctx.pre_save_pos);
    pre_save_ctx fix = ctx.pre_save[ctx.pre_save_pos];
    printf("   pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("  full: %d expected: %d\n", full, fix.expected_full);
    assert(full == fix.expected_full);
    printf("length: %zu expected: %zu\n", length, fix.expected_length);
    assert(length == fix.expected_length);
    ctx.pre_save_pos++;
    return fix.return_file;
}

int post_save(struct pvl *pvl, int full, size_t length, FILE *file, int failed) {
    printf("\n[post_save] [%d] \n", ctx.post_save_pos);
    post_save_ctx fix = ctx.post_save[ctx.post_save_pos];
    printf("   pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("  full: %d expected: %d\n", full, fix.expected_full);
    assert(full == fix.expected_full);
    printf("length: %zu expected: %zu\n", length, fix.expected_length);
    assert(length == fix.expected_length);
    printf("  file: %p expected: %p\n", (void*) file, (void*) fix.expected_file);
    assert(file == fix.expected_file);
    printf("failed: %d expected: %d\n", failed, fix.expected_failed);
    assert(failed == fix.expected_failed);
    ctx.post_save_pos++;
    return fix.return_int;
}

void leak(struct pvl *pvl, void *start, size_t length) {
    printf("\n    [leak] [%d] \n", ctx.leak_pos);
    leak_ctx fix = ctx.leak[ctx.leak_pos];
    printf("    pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("  start: %p expected: %p\n", start, fix.expected_start);
    assert(start == fix.expected_start);
    printf(" length: %zu expected: %zu\n", length, fix.expected_length);
    assert(length == fix.expected_length);
    ctx.leak_pos++;
    return;
}


void test_basic_commit() {
    printf("\n[test_basic_commit]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);

    ctx.pre_load[0].return_file = ctx.dyn_file;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 0;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = ctx.dyn_file;
    ctx.post_load[0].expected_failed = 0;
    ctx.post_load[0].expected_last_good = written_length;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, pre_load, post_load, NULL, NULL, NULL);

    assert(ctx.pre_load_pos == 1);
    assert(ctx.post_load_pos == 1);

    // Verify it
    for (size_t i = 0; i < 1024; i++) {
        if (i < 64) {
            assert(ctx.buf[i] == 1);
        } else {
            assert(ctx.buf[i] == 0);
        }
    }

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_basic_commit_mirror() {
    printf("\n[test_basic_commit_mirror]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);
    memset(ctx.mirror, 0, 1024);

    ctx.pre_load[0].return_file = ctx.dyn_file;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 0;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = ctx.dyn_file;
    ctx.post_load[0].expected_failed = 0;
    ctx.post_load[0].expected_last_good = written_length;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, pre_load, post_load, NULL, NULL, NULL);

    assert(ctx.pre_load_pos == 1);
    assert(ctx.post_load_pos == 1);

    // Verify it
    for (size_t i = 0; i < 1024; i++) {
        if (i < 64) {
            assert(ctx.buf[i] == 1);
        } else {
            assert(ctx.buf[i] == 0);
        }
    }

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_pvl_init_initial_load_error_fail_01() {
    printf("\n[test_pvl_init_initial_load_error_fail_01]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);
    memset(ctx.mirror, 0, 1024);

    // Read from a capped buffer
    FILE *load_src = fmemopen(ctx.dyn_buf,1,"r");
    printf("%d\n", errno);
    assert(load_src);

    ctx.pre_load[0].return_file = load_src;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 0;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = load_src;
    ctx.post_load[0].expected_failed = 1;
    ctx.post_load[0].expected_last_good = 0;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, pre_load, post_load, NULL, NULL, NULL);
    assert(ctx.pvl == NULL);

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_pvl_init_initial_load_error_recover_01() {
    printf("\n[test_pvl_init_initial_load_error_recover_01]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);
    memset(ctx.mirror, 0, 1024);

    // Read from a capped buffer
    FILE *load_src = fmemopen(ctx.dyn_buf,1,"r");
    printf("%d\n", errno);
    assert(load_src);

    ctx.pre_load[0].return_file = load_src;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 1;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = load_src;
    ctx.post_load[0].expected_failed = 1;
    ctx.post_load[0].expected_last_good = 0;

    ctx.pre_load[1].return_file = ctx.dyn_file;
    ctx.pre_load[1].expected_pvl = ctx.pvl;
    ctx.pre_load[1].expected_initial = 1;
    ctx.pre_load[1].expected_up_to_src = NULL;
    ctx.pre_load[1].expected_up_to_pos = 0;

    ctx.post_load[1].return_int = 0;
    ctx.post_load[1].expected_pvl = ctx.pvl;
    ctx.post_load[1].expected_file = ctx.dyn_file;
    ctx.post_load[1].expected_failed = 0;
    ctx.post_load[1].expected_last_good = written_length;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, pre_load, post_load, NULL, NULL, NULL);
    assert(ctx.pvl);

    assert(ctx.pre_load_pos == 2);
    assert(ctx.post_load_pos == 2);

    // Verify it
    for (size_t i = 0; i < 1024; i++) {
        if (i < 64) {
            assert(ctx.buf[i] == 1);
        } else {
            assert(ctx.buf[i] == 0);
        }
    }

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_pvl_init_initial_load_error_fail_02() {
    printf("\n[test_pvl_init_initial_load_error_fail_02]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);
    memset(ctx.mirror, 0, 1024);

    // Read from a capped buffer
    // FILE *load_src = fmemopen(ctx.dyn_buf,sizeof(change_header)+sizeof(mark_header)+32,"r");
    FILE *load_src = fmemopen(ctx.dyn_buf,sizeof(change_header),"r");
    printf("%d\n", errno);
    assert(load_src);

    ctx.pre_load[0].return_file = load_src;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 0;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = load_src;
    ctx.post_load[0].expected_failed = 1;
    ctx.post_load[0].expected_last_good = 0;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, pre_load, post_load, NULL, NULL, NULL);
    assert(ctx.pvl == NULL);

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_pvl_init_initial_load_error_recover_02() {
    printf("\n[test_pvl_init_initial_load_error_recover_02]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);
    memset(ctx.mirror, 0, 1024);

    // Read from a capped buffer
    FILE *load_src = fmemopen(ctx.dyn_buf,sizeof(change_header),"r");
    printf("%d\n", errno);
    assert(load_src);

    ctx.pre_load[0].return_file = load_src;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 1;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = load_src;
    ctx.post_load[0].expected_failed = 1;
    ctx.post_load[0].expected_last_good = 0;

    ctx.pre_load[1].return_file = ctx.dyn_file;
    ctx.pre_load[1].expected_pvl = ctx.pvl;
    ctx.pre_load[1].expected_initial = 1;
    ctx.pre_load[1].expected_up_to_src = NULL;
    ctx.pre_load[1].expected_up_to_pos = 0;

    ctx.post_load[1].return_int = 0;
    ctx.post_load[1].expected_pvl = ctx.pvl;
    ctx.post_load[1].expected_file = ctx.dyn_file;
    ctx.post_load[1].expected_failed = 0;
    ctx.post_load[1].expected_last_good = written_length;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, pre_load, post_load, NULL, NULL, NULL);
    assert(ctx.pvl);

    assert(ctx.pre_load_pos == 2);
    assert(ctx.post_load_pos == 2);

    // Verify it
    for (size_t i = 0; i < 1024; i++) {
        if (i < 64) {
            assert(ctx.buf[i] == 1);
        } else {
            assert(ctx.buf[i] == 0);
        }
    }

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_pvl_init_initial_load_error_fail_03() {
    printf("\n[test_pvl_init_initial_load_error_fail_03]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);
    memset(ctx.mirror, 0, 1024);

    // Read from a capped buffer
    FILE *load_src = fmemopen(ctx.dyn_buf,sizeof(change_header)+sizeof(mark_header),"r");
    printf("%d\n", errno);
    assert(load_src);

    ctx.pre_load[0].return_file = load_src;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 0;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = load_src;
    ctx.post_load[0].expected_failed = 1;
    ctx.post_load[0].expected_last_good = 0;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, pre_load, post_load, NULL, NULL, NULL);
    assert(ctx.pvl == NULL);

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_pvl_init_initial_load_error_recover_03() {
    printf("\n[test_pvl_init_initial_load_error_recover_03]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);
    memset(ctx.mirror, 0, 1024);

    // Read from a capped buffer
    FILE *load_src = fmemopen(ctx.dyn_buf,sizeof(change_header)+sizeof(mark_header),"r");
    printf("%d\n", errno);
    assert(load_src);

    ctx.pre_load[0].return_file = load_src;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 1;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = load_src;
    ctx.post_load[0].expected_failed = 1;
    ctx.post_load[0].expected_last_good = 0;

    ctx.pre_load[1].return_file = ctx.dyn_file;
    ctx.pre_load[1].expected_pvl = ctx.pvl;
    ctx.pre_load[1].expected_initial = 1;
    ctx.pre_load[1].expected_up_to_src = NULL;
    ctx.pre_load[1].expected_up_to_pos = 0;

    ctx.post_load[1].return_int = 0;
    ctx.post_load[1].expected_pvl = ctx.pvl;
    ctx.post_load[1].expected_file = ctx.dyn_file;
    ctx.post_load[1].expected_failed = 0;
    ctx.post_load[1].expected_last_good = written_length;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, pre_load, post_load, NULL, NULL, NULL);
    assert(ctx.pvl);

    assert(ctx.pre_load_pos == 2);
    assert(ctx.post_load_pos == 2);

    // Verify it
    for (size_t i = 0; i < 1024; i++) {
        if (i < 64) {
            assert(ctx.buf[i] == 1);
        } else {
            assert(ctx.buf[i] == 0);
        }
    }

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_pvl_save_error_fail_01() {
    printf("\n[test_pvl_save_error_fail_01]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    char buffer[1024];
    FILE *save_dest = fmemopen(buffer,1,"w+");
    setbuf(save_dest, NULL);

    ctx.pre_save[0].return_file = save_dest;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = save_dest;
    ctx.post_save[0].expected_failed = 1;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    assert(!fclose(save_dest));
}

void test_pvl_save_error_fail_02() {
    printf("\n[test_pvl_save_error_fail_02]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    char buffer[1024];
    FILE *save_dest = fmemopen(buffer,sizeof(change_header),"w+");
    setbuf(save_dest, NULL);

    ctx.pre_save[0].return_file = save_dest;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = save_dest;
    ctx.post_save[0].expected_failed = 1;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    assert(!fclose(save_dest));
}

void test_pvl_save_error_fail_03() {
    printf("\n[test_pvl_save_error_fail_03]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    char buffer[1024];
    FILE *save_dest = fmemopen(buffer,sizeof(change_header)+sizeof(mark_header),"w+");
    setbuf(save_dest, NULL);

    ctx.pre_save[0].return_file = save_dest;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = save_dest;
    ctx.post_save[0].expected_failed = 1;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    assert(!fclose(save_dest));
}

void test_pvl_save_error_fail_flush() {
    printf("\n[test_pvl_save_error_fail_flush]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    char buffer[1024];
    FILE *save_dest = fmemopen(buffer,sizeof(change_header)+sizeof(mark_header),"w+");
    // Leave unbufferred so that it fails at fflush
    //setbuf(save_dest, NULL);

    ctx.pre_save[0].return_file = save_dest;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = save_dest;
    ctx.post_save[0].expected_failed = 1;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    assert(!fclose(save_dest));
}

void test_pvl_save_no_destination() {
    printf("\n[test_pvl_save_no_destination]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 64;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.pre_save[0].return_file = NULL;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = NULL;
    ctx.post_save[0].expected_failed = 1;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);
}

void test_leak_no_mirror() {
    printf("\n[test_leak_no_mirror]\n");
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, NULL, NULL, NULL, NULL, leak);
    assert(ctx.pvl == NULL);
}

void test_leak_detected() {
    printf("\n[test_leak]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 32;
    int marks_count = 10;

    ctx = (test_ctx){0};
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, leak);
    assert(ctx.pvl);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    ctx.leak[0].expected_pvl = ctx.pvl;
    ctx.leak[0].expected_start = ctx.buf+32;
    ctx.leak[0].expected_length = 32;

    // Write and commit some data
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 32));
    assert(!pvl_commit(ctx.pvl));
    assert(ctx.leak_pos == 1);

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_leak_no_leak() {
    printf("\n[test_leak]\n");
    int written_length = sizeof(change_header) + sizeof(mark_header) + 32;
    int marks_count = 10;

    ctx = (test_ctx){0};
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, leak);
    assert(ctx.pvl);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = written_length;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    memset(ctx.buf, 1, 32);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 32));
    assert(!pvl_commit(ctx.pvl));
    assert(ctx.leak_pos == 0);

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

int main() {
	{
		test_init_misalignment();
		test_init_alignment();
		test_init_zero_marks();
		test_init_null_main();
		test_init_non_null_main();
		test_init_zero_legnth();
		test_init_non_zero_legnth();
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
	}

	{
		test_pvl_init_initial_load_error_fail_01();
		test_pvl_init_initial_load_error_recover_01();

		test_pvl_init_initial_load_error_fail_02();
		test_pvl_init_initial_load_error_recover_02();

		test_pvl_init_initial_load_error_fail_03();
		test_pvl_init_initial_load_error_recover_03();

		test_pvl_save_error_fail_01();
		test_pvl_save_error_fail_02();
		test_pvl_save_error_fail_03();
		test_pvl_save_error_fail_flush();

		test_pvl_save_no_destination();

		test_leak_no_mirror();
		test_leak_detected();
		test_leak_no_leak();
	}
}
