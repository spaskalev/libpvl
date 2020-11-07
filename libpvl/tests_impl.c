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

int main() {
    //TODO

    // Cover:
    // partial saves
    // coalesced marks

    test_coalesce_no_marks();
    test_coalesce_one_mark();
    test_coalesce_many_marks();
    test_coalesce_overlapping_marks();
    test_coalesce_continuous_marks();

    // test_auto_coalesce upon mark limit

    // pvl_mark_compare
    test_pvl_mark_compare_null_null();
    test_pvl_mark_compare_first_larger();
    test_pvl_mark_compare_first_smaller();
    test_pvl_mark_compare_equal();

    return 0;
}
