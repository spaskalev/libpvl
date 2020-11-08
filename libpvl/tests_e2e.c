#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pvl.h"

typedef struct {
    size_t call_count;

    FILE*  return_file;

    pvl_t* expected_pvl;
    int    expected_initial;
    FILE*  expected_file;
    int    expected_up_to_pos;
} pre_load_ctx;

typedef struct {
    size_t call_count;

    int return_int;

    pvl_t* expected_pvl;
    FILE*  expected_file;
    int    expected_failed;
    int    expected_last_good;
} post_load_ctx;

typedef struct {
    size_t call_count;

    FILE* return_file;

    pvl_t* expected_pvl;
    int    expected_full;
    size_t expected_length;
} pre_save_ctx;

typedef struct {
    size_t call_count;

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
    pre_load_ctx  pre_load;
    post_load_ctx post_load;
    pre_save_ctx  pre_save;
    post_save_ctx post_save;
} test_ctx;

// Shared global
test_ctx ctx;

FILE* pre_load(pvl_t* pvl, int initial, FILE* up_to_src, long up_to_pos) {
    assert(pvl == ctx.pre_load.expected_pvl);
    assert(initial == ctx.pre_load.expected_initial);
    assert(up_to_src == ctx.pre_load.expected_file);
    assert(up_to_pos == ctx.pre_load.expected_up_to_pos);
    ctx.pre_load.call_count++;
    return ctx.pre_load.return_file;
}

int post_load(pvl_t* pvl, FILE* file, int failed, long last_good) {
    assert(pvl == ctx.post_load.expected_pvl);
    assert(file == ctx.post_load.expected_file);
    assert(failed == ctx.post_load.expected_failed);
    assert(last_good == ctx.post_load.expected_last_good);
    ctx.post_load.call_count++;
    return ctx.post_load.return_int;
}

FILE* pre_save(pvl_t* pvl, int full, size_t length) {
    assert(pvl == ctx.pre_save.expected_pvl);
    assert(full == ctx.pre_save.expected_full);
    assert(length == ctx.pre_save.expected_length);
    ctx.pre_save.call_count++;
    return ctx.pre_save.return_file;
}

int post_save(pvl_t* pvl, int full, size_t length, FILE* file, int failed) {
    assert(pvl == ctx.post_save.expected_pvl);
    assert(full == ctx.post_save.expected_full);
    assert(length == ctx.post_save.expected_length);
    assert(file == ctx.post_save.expected_file);
    assert(failed == ctx.post_save.expected_failed);
    ctx.post_save.call_count++;
    return ctx.post_save.return_int;
}

void test_basic() {
    int written_length = 96;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, ctx.mirror, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save.return_file = ctx.dyn_file;
    ctx.pre_save.expected_pvl = ctx.pvl;
    ctx.pre_save.expected_full = 0;
    ctx.pre_save.expected_length = written_length;

    ctx.post_save.return_int = 0;
    ctx.post_save.expected_pvl = ctx.pvl;
    ctx.post_save.expected_full = 0;
    ctx.post_save.expected_length = written_length;
    ctx.post_save.expected_file = ctx.dyn_file;
    ctx.post_save.expected_failed = 0;

    // Write and commit some data
    assert(!pvl_begin(ctx.pvl));
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save.call_count == 1);
    assert(ctx.post_save.call_count == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);

    ctx.pre_load.return_file = ctx.dyn_file;
    ctx.pre_load.expected_pvl = ctx.pvl;
    ctx.pre_load.expected_initial = 1;
    ctx.pre_load.expected_file = NULL;
    ctx.pre_load.expected_up_to_pos = 0;

    ctx.post_load.return_int = 0;
    ctx.post_load.expected_pvl = ctx.pvl;
    ctx.post_load.expected_file = ctx.dyn_file;
    ctx.post_load.expected_failed = 0;
    ctx.post_load.expected_last_good = written_length;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, pre_load, post_load, NULL, NULL, NULL);

    assert(ctx.pre_load.call_count == 1);
    assert(ctx.post_load.call_count == 1);

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

void test_basic_mirror() {
    int written_length = 96;
    int marks_count = 10;

    ctx = (test_ctx){0};

    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);

    ctx.pre_save.return_file = ctx.dyn_file;
    ctx.pre_save.expected_pvl = ctx.pvl;
    ctx.pre_save.expected_full = 0;
    ctx.pre_save.expected_length = written_length;

    ctx.post_save.return_int = 0;
    ctx.post_save.expected_pvl = ctx.pvl;
    ctx.post_save.expected_full = 0;
    ctx.post_save.expected_length = written_length;
    ctx.post_save.expected_file = ctx.dyn_file;
    ctx.post_save.expected_failed = 0;

    // Write and commit some data
    assert(!pvl_begin(ctx.pvl));
    memset(ctx.buf, 1, 64);
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    assert(ctx.pre_save.call_count == 1);
    assert(ctx.post_save.call_count == 1);

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);

    ctx.pre_load.return_file = ctx.dyn_file;
    ctx.pre_load.expected_pvl = ctx.pvl;
    ctx.pre_load.expected_initial = 1;
    ctx.pre_load.expected_file = NULL;
    ctx.pre_load.expected_up_to_pos = 0;

    ctx.post_load.return_int = 0;
    ctx.post_load.expected_pvl = ctx.pvl;
    ctx.post_load.expected_file = ctx.dyn_file;
    ctx.post_load.expected_failed = 0;
    ctx.post_load.expected_last_good = written_length;

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, marks_count, ctx.buf, 1024, NULL, pre_load, post_load, NULL, NULL, NULL);

    assert(ctx.pre_load.call_count == 1);
    assert(ctx.post_load.call_count == 1);

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

int main() {
    test_basic();
    test_basic_mirror();
}
