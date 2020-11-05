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
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    size_t marks = pvl->marks_index;
    int coalesced;
    pvl_coalesce_marks(pvl, &coalesced);
    assert(coalesced == 0);
    assert(marks == pvl->marks_index);
}

void test_coalesce_one_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 1));
    size_t marks = pvl->marks_index;
    int coalesced;
    pvl_coalesce_marks(pvl, &coalesced);
    assert(coalesced == 0);
    assert(marks == pvl->marks_index);
}

void test_coalesce_many_marks() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(2)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 2, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
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
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(2)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 2, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
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
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(2)];
    char main_mem[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 2, main_mem, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, main_mem, 10));
    assert(!pvl_mark(pvl, main_mem+10, 20));
    assert(2 == pvl->marks_index);
    int coalesced;
    pvl_coalesce_marks(pvl, &coalesced);
    assert(coalesced == 1);
    assert(1 == pvl->marks_index);
    assert(pvl->marks[0].start == main_mem);
    assert(pvl->marks[0].length == 20);
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
    return 0;
}
