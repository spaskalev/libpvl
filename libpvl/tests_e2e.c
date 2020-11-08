#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pvl.h"

typedef struct {
    FILE*  return_file;

    pvl_t* expected_pvl;
    int    expected_initial;
    FILE*  expected_file;
    int    expected_up_to_pos;
} pre_load_ctx;

typedef struct {
    int return_int;

    pvl_t* expected_pvl;
    FILE*  expected_file;
    int    expected_failed;
    long   expected_last_good;
} post_load_ctx;

typedef struct {
    FILE* return_file;

    pvl_t* expected_pvl;
    int    expected_full;
    size_t expected_length;
} pre_save_ctx;

typedef struct {
    int return_int;

    pvl_t* expected_pvl;
    int    expected_full;
    size_t expected_length;
    FILE*  expected_file;
    int    expected_failed;
} post_save_ctx;

typedef struct {
    pvl_t* pvl;
    alignas(max_align_t) char pvl_at[1024];
    alignas(max_align_t) char buf[1024];
    alignas(max_align_t) char mirror[1024];

    char*  dyn_buf;
    size_t dyn_len;
    FILE*  dyn_file;

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
} test_ctx;

// Shared global
test_ctx ctx;

FILE* pre_load(pvl_t* pvl, int initial, FILE* up_to_src, long up_to_pos) {
    pre_load_ctx fix = ctx.pre_load[ctx.pre_load_pos];
    assert(pvl == fix.expected_pvl);
    assert(initial == fix.expected_initial);
    assert(up_to_src == fix.expected_file);
    assert(up_to_pos == fix.expected_up_to_pos);
    ctx.pre_load_pos++;
    return fix.return_file;
}

int post_load(pvl_t* pvl, FILE* file, int failed, long last_good) {
    post_load_ctx fix = ctx.post_load[ctx.post_load_pos];
    assert(pvl == fix.expected_pvl);
    assert(file == fix.expected_file);
    assert(failed == fix.expected_failed);
    printf("last_good: %ld expected last_good: %ld\n", last_good, fix.expected_last_good);
    assert(last_good == fix.expected_last_good);
    ctx.post_load_pos++;
    return fix.return_int;
}

FILE* pre_save(pvl_t* pvl, int full, size_t length) {
    pre_save_ctx fix = ctx.pre_save[ctx.pre_save_pos];
    assert(pvl == fix.expected_pvl);
    assert(full == fix.expected_full);
    printf("%lu\n",length);
    assert(length == fix.expected_length);
    ctx.pre_save_pos++;
    return fix.return_file;
}

int post_save(pvl_t* pvl, int full, size_t length, FILE* file, int failed) {
    post_save_ctx fix = ctx.post_save[ctx.post_save_pos];
    assert(pvl == fix.expected_pvl);
    assert(full == fix.expected_full);
    assert(length == fix.expected_length);
    assert(file == fix.expected_file);
    assert(failed == fix.expected_failed);
    ctx.post_save_pos++;
    return fix.return_int;
}

void test_basic_commit() {
    int written_length = 96;
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

    ctx.pre_load[0].return_file = ctx.dyn_file;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_file = NULL;
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
    int written_length = 96;
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

    ctx.pre_load[0].return_file = ctx.dyn_file;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_file = NULL;
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

void test_basic_commit_partial() {
    int marks_count = 1;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = 64;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = 64;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    assert(!pvl_begin(ctx.pvl));
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 32));
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    ctx.pre_save[1].return_file = ctx.dyn_file;
    ctx.pre_save[1].expected_pvl = ctx.pvl;
    ctx.pre_save[1].expected_full = 0;
    ctx.pre_save[1].expected_length = 96;

    ctx.post_save[1].return_int = 0;
    ctx.post_save[1].expected_pvl = ctx.pvl;
    ctx.post_save[1].expected_full = 0;
    ctx.post_save[1].expected_length = 96;
    ctx.post_save[1].expected_file = ctx.dyn_file;
    ctx.post_save[1].expected_failed = 0;

    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 2);
    assert(ctx.post_save_pos == 2);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);

    ctx.pre_load[0].return_file = ctx.dyn_file;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_file = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 1;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = ctx.dyn_file;
    ctx.post_load[0].expected_failed = 0;
    ctx.post_load[0].expected_last_good = 64;

    ctx.pre_load[1].return_file = ctx.dyn_file;
    ctx.pre_load[1].expected_pvl = ctx.pvl;
    ctx.pre_load[1].expected_initial = 0;
    ctx.pre_load[1].expected_file = NULL;
    ctx.pre_load[1].expected_up_to_pos = 0;

    ctx.post_load[1].return_int = 0;
    ctx.post_load[1].expected_pvl = ctx.pvl;
    ctx.post_load[1].expected_file = ctx.dyn_file;
    ctx.post_load[1].expected_failed = 0;
    ctx.post_load[1].expected_last_good = 160;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, pre_load, post_load, NULL, NULL, NULL);

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

void test_basic_commit_partial_mirror() {
    int marks_count = 1;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = 64;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = 64;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Write and commit some data
    assert(!pvl_begin(ctx.pvl));
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 32));
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));

    assert(ctx.pre_save_pos == 1);
    assert(ctx.post_save_pos == 1);

    ctx.pre_save[1].return_file = ctx.dyn_file;
    ctx.pre_save[1].expected_pvl = ctx.pvl;
    ctx.pre_save[1].expected_full = 0;
    ctx.pre_save[1].expected_length = 96;

    ctx.post_save[1].return_int = 0;
    ctx.post_save[1].expected_pvl = ctx.pvl;
    ctx.post_save[1].expected_full = 0;
    ctx.post_save[1].expected_length = 96;
    ctx.post_save[1].expected_file = ctx.dyn_file;
    ctx.post_save[1].expected_failed = 0;

    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save_pos == 2);
    assert(ctx.post_save_pos == 2);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);
    memset(ctx.mirror, 0, 1024);

    ctx.pre_load[0].return_file = ctx.dyn_file;
    ctx.pre_load[0].expected_pvl = ctx.pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_file = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 1;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = ctx.dyn_file;
    ctx.post_load[0].expected_failed = 0;
    ctx.post_load[0].expected_last_good = 64;

    ctx.pre_load[1].return_file = ctx.dyn_file;
    ctx.pre_load[1].expected_pvl = ctx.pvl;
    ctx.pre_load[1].expected_initial = 0;
    ctx.pre_load[1].expected_file = NULL;
    ctx.pre_load[1].expected_up_to_pos = 0;

    ctx.post_load[1].return_int = 0;
    ctx.post_load[1].expected_pvl = ctx.pvl;
    ctx.post_load[1].expected_file = ctx.dyn_file;
    ctx.post_load[1].expected_failed = 0;
    ctx.post_load[1].expected_last_good = 160;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, pre_load, post_load, NULL, NULL, NULL);

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

void test_basic_rollback() {
    int written_length = 96;
    int marks_count = 10;

    ctx = (test_ctx){0};

    // To break a circular dependency :)
    pvl_t* pvl = (pvl_t*)ctx.pvl_at;

    ctx.pre_load[0].return_file = NULL;
    ctx.pre_load[0].expected_pvl = pvl;
    ctx.pre_load[0].expected_initial = 1;
    ctx.pre_load[0].expected_file = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 0;
    ctx.post_load[0].expected_pvl = pvl;
    ctx.post_load[0].expected_file = NULL;
    ctx.post_load[0].expected_failed = 0;
    ctx.post_load[0].expected_last_good = 0;

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, pre_load, post_load, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save[0].return_file = ctx.dyn_file;
    ctx.pre_save[0].expected_pvl = ctx.pvl;
    ctx.pre_save[0].expected_full = 0;
    ctx.pre_save[0].expected_length = 0;

    ctx.post_save[0].return_int = 0;
    ctx.post_save[0].expected_pvl = ctx.pvl;
    ctx.post_save[0].expected_full = 0;
    ctx.post_save[0].expected_length = written_length;
    ctx.post_save[0].expected_file = ctx.dyn_file;
    ctx.post_save[0].expected_failed = 0;

    // Second load
    ctx.pre_load[1].return_file = NULL;
    ctx.pre_load[1].expected_pvl = pvl;
    ctx.pre_load[1].expected_initial = 1;
    ctx.pre_load[1].expected_file = NULL;
    ctx.pre_load[1].expected_up_to_pos = 0;

    ctx.post_load[1].return_int = 0;
    ctx.post_load[1].expected_pvl = pvl;
    ctx.post_load[1].expected_file = NULL;
    ctx.post_load[1].expected_failed = 0;
    ctx.post_load[1].expected_last_good = 0;

    // Write some data and rollback
    assert(!pvl_begin(ctx.pvl));
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_rollback(ctx.pvl));

    assert(ctx.pre_save_pos == 0);
    assert(ctx.post_save_pos == 0);
    assert(ctx.pre_load_pos == 2);
    assert(ctx.post_load_pos == 0);

    // Verify it
    for (size_t i = 0; i < 1024; i++) {
        assert(ctx.buf[i] == 0);
    }

    assert(!fclose(ctx.dyn_file));
    free(ctx.dyn_buf);
}

void test_basic_rollback_mirror() {
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    // Write some data and rollback
    assert(!pvl_begin(ctx.pvl));
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_rollback(ctx.pvl));

    assert(ctx.pre_save_pos == 0);
    assert(ctx.post_save_pos == 0);

    assert(ctx.pre_load_pos == 0);
    assert(ctx.post_load_pos == 0);

    // Verify it
    for (size_t i = 0; i < 1024; i++) {
        assert(ctx.buf[i] == 0);
    }
}

void test_basic_rollback_partial() {
}

void test_basic_rollback_partil_mirror() {
}

int main() {
    test_basic_commit();
    test_basic_commit_mirror();

    test_basic_commit_partial();
    test_basic_commit_partial_mirror();

    test_basic_rollback();
    test_basic_rollback_mirror();

    test_basic_rollback_partial();
    test_basic_rollback_partil_mirror();
}
