#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

void test_init_leak_no_mirror_leak_cb(struct pvl *pvl, void *start, size_t length, int partial) {
    (void)(pvl);
    (void)(start);
    (void)(length);
    (void)(partial);
    // TODO set a flag, fail in the test if the flag is set
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

void test_begin_pvl_null() {
    struct pvl *pvl = NULL;
    // pass some garbage
    assert(pvl_begin(pvl));
}

void test_begin() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
}

void test_begin_twice() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(pvl_begin(pvl));
}

void test_begin_after_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, buffer, 64));
    assert(pvl_begin(pvl));
}

void test_begin_after_commit() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, buffer, 128));
    assert(!pvl_commit(pvl));
    assert(!pvl_begin(pvl));
}

void test_mark_pvl_null() {
    struct pvl *pvl = NULL;
    int data = 0;
    // pass some garbage
    assert(pvl_mark(pvl, (char*) &data, 28));
}

void test_mark_no_begin() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_mark(pvl, buffer, 64));
}

void test_mark_mark_null() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(pvl_mark(pvl, NULL, 128));
}

void test_mark_zero_length() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(pvl_mark(pvl, buffer, 0));
}

void test_mark_before_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+512, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(pvl_mark(pvl, buffer, 64));
}

void test_mark_mark_main_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer+512, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(pvl_mark(pvl, buffer+496, 64));
}

void test_mark_main_mark_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(pvl_mark(pvl, buffer+496, 64));
}

void test_mark_after_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(pvl_mark(pvl, buffer+768, 64));
}

void test_mark_start_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, buffer, 64));
}

void test_mark_middle_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, buffer+512, 64));
}

void test_mark_end_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, buffer+960, 64));
}

void test_mark_all_of_main() {
    int marks = 10;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, buffer, 1024));
}

void test_mark_extra_marks() {
    int marks = 2;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, buffer, 10));
    assert(!pvl_mark(pvl, buffer, 20));
    assert(!pvl_mark(pvl, buffer, 30));
}

void test_commit_pvl_null() {
    struct pvl *pvl = NULL;
    // pass some garbage
    assert(pvl_commit(pvl));
}

void test_commit_no_begin() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl_commit(pvl));
}

void test_commit_no_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(pvl_commit(pvl));
}

void test_commit_single_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof(1)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
    assert(!pvl_mark(pvl, buffer, 128));
    assert(!pvl_commit(pvl));
}

void test_commit_max_marks() {
    size_t marks = 10;
    alignas(max_align_t) char pvlbuf[pvl_sizeof(marks)];
    char buffer[1024];
    struct pvl *pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(!pvl_begin(pvl));
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
    assert(!pvl_begin(pvl));
    for (size_t i = 0; i < marks; i++) {
        assert(!pvl_mark(pvl, buffer+i, 1));
    }
    assert(!pvl_commit(pvl));
}

int main() {
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

    test_begin_pvl_null();
    test_begin();
    test_begin_twice();
    test_begin_after_mark();
    test_begin_after_commit();

    test_mark_pvl_null();
    test_mark_no_begin();
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
    test_commit_no_begin();
    test_commit_no_mark();
    test_commit_single_mark();
    test_commit_max_marks();
    test_commit_over_max_marks();

    return 0;
}
