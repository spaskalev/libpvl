/*
 * Copyright 2020 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * libpvl - a system prevalence library (implementation)
 */

#ifndef WARNING_DO_NOT_INCLUDE_PLV_C
#error "Do not include pvl.c. Use the header and link with object code."
#endif

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "pvl.h"

/*
 * Based on http://c-faq.com/misc/bitsets.html
 */
#define BITSET_SIZE(size) ((size + CHAR_BIT - 1) / CHAR_BIT)
#define BITSET_POS(pos) ((pos) / CHAR_BIT)
#define BITSET_MASK(pos) (1 << ((pos) % CHAR_BIT))
#define BITSET_SET(name, pos) ((name)[BITSET_POS(pos)] |= BITSET_MASK(pos))
#define BITSET_RESET(name, pos) ((name)[BITSET_POS(pos)] *= ~BITSET_MASK(pos))
#define BITSET_TEST(name, pos) ((name)[BITSET_POS(pos)] & BITSET_MASK(pos))

struct pvl {
    char               *main;
    char               *mirror;
    size_t             length;
    pre_load_callback  *pre_load_cb;
    post_load_callback *post_load_cb;
    pre_save_callback  *pre_save_cb;
    post_save_callback *post_save_cb;
    leak_callback      *leak_cb;
    FILE               *last_save_file;
    long               last_save_pos;
    _Bool              dirty;
    /*
     * The field order bellow is used by pvl_save()
     */
    size_t             span_length;
    size_t             span_count;
    char               spans[];
};

size_t pvl_sizeof(size_t span_count) {
    return sizeof(struct pvl)+(BITSET_SIZE(span_count) * sizeof(char));
}

static size_t pvl_sizeof_change_header(struct pvl *pvl);
static void *pvl_change_header(struct pvl *pvl);
static int pvl_load(struct pvl *pvl, int initial, FILE *req_from, long req_pos);
static int pvl_save(struct pvl *pvl);
static void pvl_clear_memory(struct pvl *pvl);
static void pvl_clear_spans(struct pvl *pvl);
static void pvl_detect_leaks(struct pvl *pvl);
static void pvl_detect_leaks_inner(struct pvl *pvl, size_t from, size_t to);

struct pvl *pvl_init(char *at, size_t span_count, char *main, size_t length, char *mirror,
    pre_load_callback pre_load_cb, post_load_callback post_load_cb,
    pre_save_callback pre_save_cb, post_save_callback post_save_cb,
    leak_callback leak_cb) {
    /*
     * Check for alignment
     */
    size_t alignment = ((uintptr_t) at) % alignof(max_align_t);
    if (alignment != 0) {
        return NULL;
    }

    /*
     * Leak detection requires a mirror
     */
    if ((!mirror) && leak_cb) {

        return NULL;
    }

    /*
     * Check for overlap between main and mirror blocks
     */
    if (mirror != NULL) {
        if (((mirror <= main) && ((mirror+length) > main)) ||
            ((main <= mirror) && ((main+length) > mirror))) {
            return NULL;
        }
    }

    /*
     * main must not be NULL
     */
    if (main == NULL) {
        return NULL;
    }

    /*
     * length must be positive
     */
    if (length == 0) {
        return NULL;
    }

    /*
     * span_count must be positive
     */
    if (span_count == 0) {
        return NULL;
    }

    /*
     * length must be divisible by span_count
     */
    if (length % span_count) {
        return NULL;
    }

    struct pvl *pvl = (struct pvl*) at;

    /*
     * Zero the pvl destination. The custom sizeof function will
     * account for the flexible array at the end of the structure.
     */
    memset(pvl, 0, pvl_sizeof(span_count));

    pvl->span_count = span_count;
    pvl->main = main;
    pvl->length = length;
    pvl->mirror = mirror;
    pvl->span_length = pvl->length / pvl->span_count;

    // Callbacks are checked at call sites
    pvl->pre_load_cb = pre_load_cb;
    pvl->post_load_cb = post_load_cb;

    pvl->pre_save_cb = pre_save_cb;
    pvl->post_save_cb = post_save_cb;

    pvl->leak_cb = leak_cb;

    // Perform initial load
    if (pvl_load(pvl, 1, NULL, 0) != 0) {
        return NULL;
    }

    return pvl;
}

int pvl_mark(struct pvl *pvl, char *start, size_t length) {
    if (pvl == NULL) {
        return 1;
    }

    /*
     * Validate the span
     */
    if ((length == 0) || (start < pvl->main)
            || ((start+length) > (pvl->main+pvl->length))) {
        return 1;
    }

    /*
     * Set the matching spans
     */
    size_t from_pos = (start - pvl->main) / pvl->span_length;
    size_t to_pos = ((start+length) - pvl->main) / pvl->span_length;

    for (size_t i = from_pos; i <= to_pos; i++) {
        BITSET_SET(pvl->spans, i);
    }

    pvl->dirty = 1;
    return 0;
}

int pvl_commit(struct pvl *pvl) {
    if (pvl == NULL) {
        return 1;
    }

    /*
     * Commit with no marks is a no-op
     */
    if (! pvl->dirty) {
        return 0;
    }

    return pvl_save(pvl);
}

static void pvl_clear_memory(struct pvl *pvl) {
    memset(pvl->main, 0, pvl->length);
    if (pvl->mirror) {
        memset(pvl->mirror, 0, pvl->length);
    }
}

static void pvl_clear_spans(struct pvl *pvl) {
    memset(pvl->spans, 0, (BITSET_SIZE(pvl->span_count) * sizeof(char)));
}

static size_t pvl_sizeof_change_header(struct pvl *pvl) {
    ptrdiff_t base_size = (pvl->spans) - (char*)(&pvl->span_length);
    size_t header_size = base_size + (BITSET_SIZE(pvl->span_count) * sizeof(char));
    return header_size;
}

static void *pvl_change_header(struct pvl *pvl) {
    return &pvl->span_length;
}

/*
 * Save the currently-marked memory content
 */
static int pvl_save(struct pvl *pvl) {
    /*
     * Perform leak detection
     */
    if (pvl->leak_cb) {
        pvl_detect_leaks(pvl);
    }

    /*
     * Early return if there is no save handler
     */
    if (pvl->pre_save_cb == NULL) {
        return 0;
    }

    /*
     * Calculate the total size of the change
     */
    size_t content_size = 0;
    for (size_t i = 0; i < pvl->span_count; i++) {
        if (BITSET_TEST(pvl->spans, i)) {
            content_size += pvl->span_length;
        }
    }
    size_t header_size = pvl_sizeof_change_header(pvl);
    size_t total = header_size + content_size;

    /*
     * Determine whether this is a full change
     */
    int full = (total == pvl->length);

    /*
     * Get the save destination through the pre-save callback
     */
    FILE *file = NULL;
    file = (*pvl->pre_save_cb)(pvl, full, total);

    /*
     * Fail if the callback did not provide a place to save
     */
    if (file == NULL) {
        (pvl->post_save_cb)(pvl, full, total, file, 1);
        return 1;
    }

    /*
     * Construct and save the header
     */
    if (fwrite(pvl_change_header(pvl), header_size, 1, file) != 1) {
        (pvl->post_save_cb)(pvl, full, total, file, 1);
        return 1;
    }

    /*
     * Save each span
     *
     * TODO: optimize for continuous spans
     */
    for (size_t i = 0; i < pvl->span_count; i++) {
        if (BITSET_TEST(pvl->spans, i)) {
            if (fwrite(pvl->main + (i*pvl->span_length), pvl->span_length, 1, file) != 1) {
                (pvl->post_save_cb)(pvl, full, total, file, 1);
                return 1;
            }
        }
    }

    /*
     * Flush, as buffered data may otherwise be lost
     */
    if (fflush(file)) {
        (pvl->post_save_cb)(pvl, full, total, file, 1);
        return 1;
    }

    /*
     * Apply to mirror
     */
    if (pvl->mirror) {
        for (size_t i = 0; i < pvl->span_count; i++) {
            if (BITSET_TEST(pvl->spans, i)) {
                memcpy(pvl->mirror + (i*pvl->span_length), pvl->main + (i*pvl->span_length), pvl->span_length);
            }
        }
    }

    /*
     * Signal that the save is done
     */
    (pvl->post_save_cb)(pvl, full, total, file, 0);

    pvl_clear_spans(pvl);
    pvl->dirty = 0;
    return 0;
}

static int pvl_load(struct pvl *pvl, int initial, FILE *up_to_src, long up_to_pos) {
    /*
     * Cannot load anything if there is no pre-load callback
     */
    if (! pvl->pre_load_cb) {
        return 0;
    }

    FILE *file;
    long last_good_pos = 0;
    int reset_load = 0;

    read_loop:
    while (1) {
        if (reset_load) {
            initial = 1;
            reset_load = 0;
            pvl_clear_memory(pvl);
        }

        /*
         * Call the pre-load callback.
         */
        file = (pvl->pre_load_cb)(pvl, initial, up_to_src, up_to_pos);

        if (initial) {
            // Only the first read in a sequence should be indicated as initial
            initial = 0;
        }

        if (!file) {
            // Nothing to load
            return 0;
        }

        /*
         * Try to load a change from it
         */
        last_good_pos = ftell(file);
        size_t header_size = pvl_sizeof_change_header(pvl);
        if (fread(pvl_change_header(pvl), header_size, 1, file) != 1) {
            // Couldn't load the change, signal the callback
            if ((pvl->post_load_cb)(pvl, file, 1, last_good_pos)) {
                reset_load = 1;
                goto read_loop;
            }
            return 1;
        }

        /*
         * Read each span
         */
        for (size_t i = 0; i < pvl->span_count; i++) {
            if (BITSET_TEST(pvl->spans, i)) {
                if (fread(pvl->main + (i*pvl->span_length), pvl->span_length, 1, file) != 1) {
                    // Couldn't load the change, signal the callback
                    if ((pvl->post_load_cb)(pvl, file, 1, last_good_pos)) {
                        reset_load = 1;
                        goto read_loop;
                    }
                    return 1;
                }
            }
        }
        pvl_clear_spans(pvl);
        last_good_pos = ftell(file);

        /*
         * Apply it to the mirror.
         * TODO - optimize this only for changed spans
         */
        if (pvl->mirror) {
            memcpy(pvl->mirror, pvl->main, pvl->length);
        }

        /*
         * Report success via the post-load callback
         */
        if ((pvl->post_load_cb)(pvl, file, 0, last_good_pos)) {
            goto read_loop;
        }
        return 0;
    }
}

static void pvl_detect_leaks(struct pvl *pvl) {
    /*
     * Continuous span optimization is not required here :)
     */
    for (size_t i = 0; i < pvl->span_count; i++) {
        if (! BITSET_TEST(pvl->spans, i)) {
            pvl_detect_leaks_inner(pvl, (i*pvl->span_length), (i*pvl->span_length) + pvl->span_length);
        }
    }
}

static void pvl_detect_leaks_inner(struct pvl *pvl, size_t from, size_t to) {
    size_t in_diff = 0;
    size_t diff_start = 0;
    for (size_t i = from; i <= to; i++) {
        if (in_diff) {
            if (pvl->main[i] == pvl->mirror[i]) { //diff over
                // report diff
                (pvl->leak_cb)(pvl, (pvl->main)+diff_start, i-diff_start);
                // clear trackers
                in_diff = 0;
                diff_start = 0;
            } // else do nothing
        } else {
            if (pvl->main[i] != pvl->mirror[i]) { //diff starts
                // set trackers
                in_diff = 1;
                diff_start = i;
            } // else do nothing
        }
    }
}
