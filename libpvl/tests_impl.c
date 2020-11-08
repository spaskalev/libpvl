#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * Define the guard to test implementation details
 */
#define WARNING_DO_NOT_INCLUDE_PLV_C
#include "pvl.c"

#include "tests_common.h"

void test_coalesce_no_marks() {
    size_t mark_count = 1;
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(mark_count)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    size_t marks = pvl->marks_index;
    int coalesced;
    pvl_coalesce_marks(pvl, &coalesced);
    assert(coalesced == 0);
    assert(marks == pvl->marks_index);
}

void test_coalesce_one_mark() {
    size_t mark_count = 1;
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(mark_count)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 1));
    size_t marks = pvl->marks_index;
    int coalesced;
    pvl_coalesce_marks(pvl, &coalesced);
    assert(coalesced == 0);
    assert(marks == pvl->marks_index);
}

void test_coalesce_many_marks() {
    size_t mark_count = 2;
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(mark_count)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 1));
    assert(!pvl_mark(pvl, main_mem+10, 1));
    assert(2 == pvl->marks_index);
    int coalesced;
    pvl_coalesce_marks(pvl, &coalesced);
    assert(coalesced == 0);
    assert(2 == pvl->marks_index);
}

void test_coalesce_overlapping_marks() {
    size_t mark_count = 3;
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(mark_count)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 10));
    assert(!pvl_mark(pvl, main_mem+5, 15));
    assert(2 == pvl->marks_index);
    int coalesced;
    pvl_coalesce_marks(pvl, &coalesced);
    assert(coalesced == 1);
    assert(1 == pvl->marks_index);
    assert(pvl->marks[0].start == main_mem);
    assert(pvl->marks[0].length == 15);
}

void test_coalesce_continuous_marks() {
    size_t mark_count = 3;
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(mark_count)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, mark_count, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 10));
    assert(!pvl_mark(pvl, main_mem+10, 20));
    assert(!pvl_mark(pvl, main_mem+100, 20));
    assert(mark_count == pvl->marks_index);
    int coalesced;
    pvl_coalesce_marks(pvl, &coalesced);
    assert(coalesced == 1);
    assert(2 == pvl->marks_index);
    assert(pvl->marks[0].start == main_mem);
    assert(pvl->marks[0].length == 20);
    assert(pvl->marks[1].start == main_mem+100);
    assert(pvl->marks[1].length == 20);
}

void test_pvl_mark_compare_null_null() {
    pvl_mark_t a, b;
    a = (pvl_mark_t){0};
    b = (pvl_mark_t){0};
    assert(pvl_mark_compare(&a, &b) == 0);
}

void test_pvl_mark_compare_first_larger() {
    char buf[512];
    pvl_mark_t a, b;
    a = (pvl_mark_t){0};
    b = (pvl_mark_t){0};

    a.start = buf+512;
    b.start = buf;
    assert(pvl_mark_compare(&a, &b) == 1);
}

void test_pvl_mark_compare_first_smaller() {
    char buf[512];
    pvl_mark_t a, b;
    a = (pvl_mark_t){0};
    b = (pvl_mark_t){0};

    a.start = buf;
    b.start = buf+512;
    assert(pvl_mark_compare(&a, &b) == -1);
}

void test_pvl_mark_compare_equal() {
    char buf[512];
    pvl_mark_t a, b;
    a = (pvl_mark_t){0};
    b = (pvl_mark_t){0};

    a.start = buf;
    b.start = buf;
    assert(pvl_mark_compare(&a, &b) == 0);
}

void test_pvl_save_fwrite_failure() {
    printf("\n[test_basic_commit]\n");
    int written_length = 96;
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

    //assert(!fclose(ctx.dyn_file));
    //free(ctx.dyn_buf);
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

    test_pvl_save_fwrite_failure();

    return 0;
}
