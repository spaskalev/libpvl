typedef struct {
    FILE*  return_file;

    pvl_t* expected_pvl;
    int    expected_initial;
    FILE*  expected_up_to_src;
    long   expected_up_to_pos;
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
    printf("\n [pre_load] [%d] \n", ctx.pre_load_pos);
    pre_load_ctx fix = ctx.pre_load[ctx.pre_load_pos];
    printf("      pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("  initial: %d expected: %d\n", initial, fix.expected_initial);
    assert(initial == fix.expected_initial);
    printf("up_to_src: %p expected: %p\n", (void*) up_to_src, (void*) fix.expected_up_to_src);
    assert(up_to_src == fix.expected_up_to_src);
    printf("up_to_pos: %ld expected: %ld\n", up_to_pos, fix.expected_up_to_pos);
    assert(up_to_pos == fix.expected_up_to_pos);
    ctx.pre_load_pos++;
    return fix.return_file;
}

int post_load(pvl_t* pvl, FILE* file, int failed, long last_good) {
    printf("\n[post_load] [%d] \n", ctx.post_load_pos);
    post_load_ctx fix = ctx.post_load[ctx.post_load_pos];
    printf("      pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("     file: %p expected: %p\n", (void*) file, (void*) fix.expected_file);
    assert(file == fix.expected_file);
    printf("   failed: %d expected: %d\n", failed, fix.expected_failed);
    assert(failed == fix.expected_failed);
    printf("last_good: %ld expected: %ld\n", last_good, fix.expected_last_good);
    assert(last_good == fix.expected_last_good);
    ctx.post_load_pos++;
    return fix.return_int;
}

FILE* pre_save(pvl_t* pvl, int full, size_t length) {
    printf("\n [pre_save] [%d] \n", ctx.pre_save_pos);
    pre_save_ctx fix = ctx.pre_save[ctx.pre_save_pos];
    printf("   pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("  full: %d expected: %d\n", full, fix.expected_full);
    assert(full == fix.expected_full);
    printf("length: %ld expected: %ld\n", length, fix.expected_length);
    assert(length == fix.expected_length);
    ctx.pre_save_pos++;
    return fix.return_file;
}

int post_save(pvl_t* pvl, int full, size_t length, FILE* file, int failed) {
    printf("\n[post_save] [%d] \n", ctx.post_save_pos);
    post_save_ctx fix = ctx.post_save[ctx.post_save_pos];
    printf("   pvl: %p expected: %p\n", (void*) pvl, (void*) fix.expected_pvl);
    assert(pvl == fix.expected_pvl);
    printf("  full: %d expected: %d\n", full, fix.expected_full);
    assert(full == fix.expected_full);
    printf("length: %ld expected: %ld\n", length, fix.expected_length);
    assert(length == fix.expected_length);
    printf("  file: %p expected: %p\n", (void*) file, (void*) fix.expected_file);
    assert(file == fix.expected_file);
    printf("failed: %d expected: %d\n", failed, fix.expected_failed);
    assert(failed == fix.expected_failed);
    ctx.post_save_pos++;
    return fix.return_int;
}
