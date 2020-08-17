#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pvl.h"

int test_init_misalignment() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)+1];
    char main[1024];
    // Ensure wrong alignment (max_align_t+1)
    pvl_t* pvl = pvl_init(pvlbuf+1, 1, main, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert (pvl == NULL);
}

int test_init_alignment() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char main[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, main, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

int test_init_zero_marks() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(0)];
    char main[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 0, main, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

int test_init_null_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    pvl_t* pvl = pvl_init(pvlbuf, 1, NULL, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

int test_init_non_null_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char main[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, main, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

int test_init_zero_legnth() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char main[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, main, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

int test_init_non_zero_legnth() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char main[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, main, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

int test_init_main_mirror_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 512, buffer+256, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

int test_init_main_mirror_no_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 512, buffer+512, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

int test_init_mirror_main_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer+256, 512, buffer, NULL, NULL, NULL, NULL, NULL);
    assert(pvl == NULL);
}

int test_init_mirror_main_no_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer+512, 512, buffer, NULL, NULL, NULL, NULL, NULL);
    assert(pvl != NULL);
}

void test_init_leak_no_mirror_leak_cb(pvl_t* pvl, void* start, size_t length) {
    // TODO set a flag, fail in the test if the flag is set
}

int test_init_leak_no_mirror() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL,
        test_init_leak_no_mirror_leak_cb);
    assert(pvl == NULL);
}

char* test_save_dynbuf = NULL;
size_t test_save_dynlen = 0;
FILE* test_save_dynfile = NULL;

int test_save_pre_save_cb(pvl_t* pvl, size_t length, FILE** file) {
    test_save_dynfile = open_memstream(&test_save_dynbuf, &test_save_dynlen);
    *file = test_save_dynfile;
}

int test_save_post_save_cb(pvl_t* pvl, size_t length, FILE* file) {
    fflush(file);
    assert(length == test_save_dynlen);
    fclose(test_save_dynfile);
    free(test_save_dynbuf);
}

int test_save() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL,
        test_save_pre_save_cb, test_save_post_save_cb, NULL);
    memcpy(buffer, "Hello world\0", 12);
    int op_result = 0;
    op_result = pvl_mark(pvl, buffer, 12);
    assert(op_result == 0);
    pvl_commit(pvl);
    assert(op_result == 0);
}

int test_mark_pvl_null() {
    pvl_t* pvl = NULL;
    int data = 0;
    // pass some garbage
    int result = pvl_mark(pvl, (char*) &data, 28);
    assert(result != 0);
}

int test_mark_mark_null() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, NULL, 128);
    assert(result != 0);
}

int test_mark_zero_length() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer, 0);
    assert(result != 0);
}

int test_mark_before_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer+512, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer, 64);
    assert(result != 0);
}

int test_mark_mark_main_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer+512, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer+496, 64);
    assert(result != 0);
}

int test_mark_main_mark_overlap() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer+496, 64);
    assert(result != 0);
}

int test_mark_after_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 512, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer+768, 64);
    assert(result != 0);
}

int test_mark_start_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer, 64);
    assert(result == 0);
}

int test_mark_middle_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer+512, 64);
    assert(result == 0);
}

int test_mark_end_of_main() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer+960, 64);
    assert(result == 0);
}

int test_mark_all_of_main() {
    int marks = 10;
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(marks)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_mark(pvl, buffer, 1024);
    assert(result == 0);
}

int test_commit_pvl_null() {
    pvl_t* pvl = NULL;
    int data = 0;
    // pass some garbage
    int result = pvl_commit(pvl);
    assert(result != 0);
}

int test_commit_no_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_commit(pvl);
    assert(result == 0);
}

int test_commit_single_mark() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(0 == pvl_mark(pvl, buffer, 128));
    int result = pvl_commit(pvl);
    assert(result == 0);
}

int test_commit_max_marks() {
    int marks = 10;
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(marks)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, marks, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    for (size_t i = 0; i < marks; i++) {
        assert(0 == pvl_mark(pvl, buffer+i, 1));
    }
    int result = pvl_commit(pvl);
    assert(result == 0);
}

int test_commit_over_max_marks() {
    int marks = 10;
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(marks)];
    char buffer[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, NULL, NULL, NULL, NULL, NULL, NULL);
    for (size_t i = 0; i < marks; i++) {
        assert(0 == pvl_mark(pvl, buffer+i, 1));
    }
    int result = pvl_commit(pvl);
    assert(result == 0);
}

int test_rollback_pvl_null() {
    pvl_t* pvl = NULL;
    int data = 0;
    // pass some garbage
    int result = pvl_rollback(pvl);
    assert(result != 0);
}

int test_rollback_no_mark_mirror() {
    alignas(max_align_t) char pvlbuf[pvl_sizeof_pvl_t(1)];
    char buffer[1024];
    char mirror[1024];
    pvl_t* pvl = pvl_init(pvlbuf, 1, buffer, 1024, mirror, NULL, NULL, NULL, NULL, NULL);
    int result = pvl_rollback(pvl);
    assert(result == 0);
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

    test_commit_pvl_null();
    test_commit_no_mark();
    test_commit_single_mark();
    test_commit_max_marks();
    test_commit_over_max_marks();

    test_rollback_pvl_null();
    /* test_rollback_no_mark_mirror();
    test_rollback_single_mark_mirror();
    test_rollback_max_marks_mirror();
    test_rollback_over_max_marks_mirror();
    test_rollback_no_mark_no_mirror();
    test_rollback_single_mark_no_mirror();
    test_rollback_max_marks_no_mirror();
    test_rollback_over_max_marks_no_mirror();
    */

    /*
    test_mirror_content();
    test_mirror_rollback();

    test_leak_mirror();
    test_leak_no_mirror();
    */

    test_save();
}
