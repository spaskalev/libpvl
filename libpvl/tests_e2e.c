#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pvl.h"

#include "tests_common.h"

void test_basic_commit() {
    printf("\n[test_basic_commit]\n");
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

void test_basic_commit_partial() {
    printf("\n[test_basic_commit_partial]\n");
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
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 1;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = ctx.dyn_file;
    ctx.post_load[0].expected_failed = 0;
    ctx.post_load[0].expected_last_good = 64;

    ctx.pre_load[1].return_file = ctx.dyn_file;
    ctx.pre_load[1].expected_pvl = ctx.pvl;
    ctx.pre_load[1].expected_initial = 0;
    ctx.pre_load[1].expected_up_to_src = NULL;
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
    printf("\n[test_basic_commit_partial_mirror]\n");
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
    ctx.pre_load[0].expected_up_to_src = NULL;
    ctx.pre_load[0].expected_up_to_pos = 0;

    ctx.post_load[0].return_int = 1;
    ctx.post_load[0].expected_pvl = ctx.pvl;
    ctx.post_load[0].expected_file = ctx.dyn_file;
    ctx.post_load[0].expected_failed = 0;
    ctx.post_load[0].expected_last_good = 64;

    ctx.pre_load[1].return_file = ctx.dyn_file;
    ctx.pre_load[1].expected_pvl = ctx.pvl;
    ctx.pre_load[1].expected_initial = 0;
    ctx.pre_load[1].expected_up_to_src = NULL;
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

int main() {
    test_basic_commit();
    test_basic_commit_mirror();

    test_basic_commit_partial();
    test_basic_commit_partial_mirror();
}
