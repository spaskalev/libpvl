#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pvl.h"

typedef struct {
    pvl_t* pvl;
    alignas(max_align_t) char pvl_at[1024];
    alignas(max_align_t) char buf[1024];

    char*  dyn_buf;
    size_t dyn_len;
    FILE*  dyn_file;

    size_t expected_save_length;
} test_ctx;

// Shared global
test_ctx ctx;

FILE* pre_load(pvl_t* pvl, int initial, FILE* up_to_src, long up_to_pos) {
    (void)(pvl);
    (void)(initial);
    (void)(up_to_src);
    (void)(up_to_pos);

    assert(pvl == ctx.pvl);
    assert(initial == 1);
    assert(up_to_src == NULL);
    assert(up_to_pos == 0);

    return ctx.dyn_file;
}

int post_load(pvl_t* pvl, FILE* file, int failed, long fpos) {
    (void)(pvl);
    (void)(file);
    (void)(failed);
    (void)(fpos);

    assert(pvl == ctx.pvl);
    assert(file == ctx.dyn_file);
    assert(failed == 0);
    return 0;
}

FILE* pre_save(pvl_t* pvl, int full, size_t length) {
    (void)(pvl);
    (void)(full);
    (void)(length);

    assert(pvl == ctx.pvl);
    assert(full == 0);
    assert(length >= ctx.expected_save_length);

    ctx.dyn_file = open_memstream(&ctx.dyn_buf, &ctx.dyn_len);
    return ctx.dyn_file;
}

int post_save(pvl_t* pvl, int full, size_t length, FILE* file, int failed) {
    (void)(pvl);
    (void)(full);
    (void)(length);
    (void)(file);
    (void)(failed);

    assert(pvl == ctx.pvl);
    assert(full == 0);
    assert(length >= ctx.expected_save_length);
    assert(file == ctx.dyn_file);
    assert(failed == 0);

    return 0;
}

void test_basic() {
    ctx = (test_ctx){0};
    ctx.pvl = pvl_init(ctx.pvl_at, 10, ctx.buf, 1024, NULL, NULL, NULL, pre_save, post_save, NULL);
    assert(ctx.pvl != NULL);

    // Write and commit some data
    assert(!pvl_begin(ctx.pvl));
    memset(ctx.buf, 1, 64);
    ctx.expected_save_length = 64;
    assert(!pvl_mark(ctx.pvl, ctx.buf, 64));
    assert(!pvl_commit(ctx.pvl));

    // Prepare for reading the data
    rewind(ctx.dyn_file);
    memset(ctx.pvl_at, 0, 1024);
    memset(ctx.buf, 0, 1024);

    // Create a new pvl to load the data
    ctx.pvl = pvl_init(ctx.pvl_at, 10, ctx.buf, 1024, NULL, pre_load, post_load, NULL, NULL, NULL);

    // Verify it
    for (size_t i = 0; i < 1024; i++) {
        if (i < 64) {
            assert(ctx.buf[i] == 1);
        } else {
            assert(ctx.buf[i] == 0);
        }
    }
}

int main() {
    test_basic();
}
