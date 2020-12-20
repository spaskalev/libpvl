#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/*
 * Define the guard to test implementation details
 */
#define WARNING_DO_NOT_INCLUDE_PLV_C
#include "pvl.c"

#include "tests_common.h"

void test_coalesce_no_marks() {
    size_t mark_count = 1;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(mark_count)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    size_t marks = pvl->marks_index;
    assert(!pvl_coalesce_marks(pvl));
    assert(marks == pvl->marks_index);
}

void test_coalesce_one_mark() {
    size_t mark_count = 1;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(mark_count)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 1));
    size_t marks = pvl->marks_index;
    assert(!pvl_coalesce_marks(pvl));
    assert(marks == pvl->marks_index);
}

void test_coalesce_many_marks() {
    size_t mark_count = 2;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(mark_count)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 1));
    assert(!pvl_mark(pvl, main_mem+10, 1));
    assert(2 == pvl->marks_index);
    assert(!pvl_coalesce_marks(pvl));
    assert(2 == pvl->marks_index);
}

void test_coalesce_overlapping_marks() {
    size_t mark_count = 3;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(mark_count)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 10));
    assert(!pvl_mark(pvl, main_mem+5, 15));
    assert(2 == pvl->marks_index);
    assert(pvl_coalesce_marks(pvl));
    assert(1 == pvl->marks_index);
    assert(pvl->marks[0].start == main_mem);
    assert(pvl->marks[0].length == 15);
}

void test_coalesce_continuous_marks() {
    size_t mark_count = 3;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(mark_count)];
    char main_mem[1024];
    struct pvl *pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 10));
    assert(!pvl_mark(pvl, main_mem+10, 20));
    assert(!pvl_mark(pvl, main_mem+100, 20));
    assert(mark_count == pvl->marks_index);
    assert(pvl_coalesce_marks(pvl));
    assert(2 == pvl->marks_index);
    assert(pvl->marks[0].start == main_mem);
    assert(pvl->marks[0].length == 20);
    assert(pvl->marks[1].start == main_mem+100);
    assert(pvl->marks[1].length == 20);
}

void test_pvl_mark_compare_null_null() {
    mark a, b;
    a = (mark){0};
    b = (mark){0};
    assert(pvl_mark_compare(&a, &b) == 0);
}

void test_pvl_mark_compare_first_larger() {
    char buf[512];
    mark a, b;
    a = (mark){0};
    b = (mark){0};

    a.start = buf+512;
    b.start = buf;
    assert(pvl_mark_compare(&a, &b) == 1);
}

void test_pvl_mark_compare_first_smaller() {
    char buf[512];
    mark a, b;
    a = (mark){0};
    b = (mark){0};

    a.start = buf;
    b.start = buf+512;
    assert(pvl_mark_compare(&a, &b) == -1);
}

void test_pvl_mark_compare_equal() {
    char buf[512];
    mark a, b;
    a = (mark){0};
    b = (mark){0};

    a.start = buf;
    b.start = buf;
    assert(pvl_mark_compare(&a, &b) == 0);
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
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
    ctx.leak[0].expected_partial = 0;

    // Write and commit some data
    assert(!pvl_begin(ctx.pvl));
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
    assert(!pvl_begin(ctx.pvl));
    memset(ctx.buf, 1, 32);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 32));
    assert(!pvl_commit(ctx.pvl));
    assert(ctx.leak_pos == 0);

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

int main() {

    test_coalesce_no_marks();
    test_coalesce_one_mark();
    test_coalesce_many_marks();
    test_coalesce_overlapping_marks();
    test_coalesce_continuous_marks();

    test_pvl_mark_compare_null_null();
    test_pvl_mark_compare_first_larger();
    test_pvl_mark_compare_first_smaller();
    test_pvl_mark_compare_equal();

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

    return 0;
}
