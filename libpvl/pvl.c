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
    /* read context and callback */
    void               *read_ctx;
    read_callback      *read_cb;
    /* write context and callback */
    void               *write_ctx;
    write_callback     *write_cb;
    /* leak detection context and callback */
    void               *leak_ctx;
    leak_callback      *leak_cb;

    _Bool              dirty;
    /*
     * The field order bellow is used by
     * pvl_sizeof_change_header() and pvl_change_header()
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
static size_t pvl_marked_size(struct pvl *pvl);
static int pvl_load(struct pvl *pvl);
static int pvl_save(struct pvl *pvl);
static void pvl_clear_spans(struct pvl *pvl);
static void pvl_detect_leaks(struct pvl *pvl);
static void pvl_detect_leaks_inner(struct pvl *pvl, size_t from, size_t to);

struct pvl *pvl_init(char *at, size_t span_count,
    char *main, size_t length, char *mirror,
    void *read_ctx, read_callback read_cb,
    void *write_ctx, write_callback write_cb,
    void *leak_ctx, leak_callback leak_cb) {
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

    /*
     * Callbacks are checked at call sites
     */
    pvl->read_ctx = read_ctx;
    pvl->read_cb = read_cb;

    pvl->write_ctx = write_ctx;
    pvl->write_cb = write_cb;

    pvl->leak_ctx = leak_ctx;
    pvl->leak_cb = leak_cb;

    /*
     * Perform initial load
     */
    if (pvl_load(pvl) != 0) {
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

static size_t pvl_marked_size(struct pvl *pvl) {
    size_t content_size = 0;
    for (size_t i = 0; i < pvl->span_count; i++) {
        if (BITSET_TEST(pvl->spans, i)) {
            content_size += pvl->span_length;
        }
    }
    return content_size;
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
     * Early return if there is no write callback
     */
    if (pvl->write_cb == NULL) {
        return 0;
    }

    size_t content_size = pvl_marked_size(pvl);
    size_t header_size = pvl_sizeof_change_header(pvl);

    /*
     * Construct and save the header
     */
    if (pvl->write_cb(pvl->write_ctx, pvl_change_header(pvl), header_size, content_size)) {
        return 1;
    }

    /*
     * Save each span
     *
     * TODO: optimize for continuous spans
     */
    for (size_t i = 0; i < pvl->span_count; i++) {
        if (BITSET_TEST(pvl->spans, i)) {
            content_size -= pvl->span_length;
            if(pvl->write_cb(pvl->write_ctx, pvl->main + (i*pvl->span_length), pvl->span_length, content_size)) {
                return 1;
            }
        }
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

    pvl_clear_spans(pvl);
    pvl->dirty = 0;
    return 0;
}

static int pvl_load(struct pvl *pvl) {
    /*
     * Cannot load anything if there is no read callback
     */
    if (! pvl->read_cb) {
        return 0;
    }

    while (1) {
        /*
         * Try to read a change
         */
        int read_result;
        size_t header_size = pvl_sizeof_change_header(pvl);
        read_result = pvl->read_cb(pvl->read_ctx, pvl_change_header(pvl), header_size, 0);
        if (read_result == EOF) {
            /* The read callback has successfully read all (or none) changes */
            break;
        }
        if (read_result) {
            /*
             * The read callback failed to read a subsequent change but the pvl memory state
             * is not disrupted - operation can continue.
             */
            break;
        }

        size_t content_size = pvl_marked_size(pvl);
        read_result = pvl->read_cb(pvl->read_ctx, NULL, 0, content_size);
        if (read_result != 0) {
            /*
             * The read callback indicated that it cannot serve the remaining bytes
             * but the pvl memory state is not disrupted - operation can continue.
             */
             break;
        }

        /*
         * Read each span
         */
        for (size_t i = 0; i < pvl->span_count; i++) {
            if (BITSET_TEST(pvl->spans, i)) {
                content_size -= pvl->span_length;
                /*
                 * At this point read should always succeed up to the remaining bytes.
                 */
                if (pvl->read_cb(pvl->read_ctx, pvl->main + (i*pvl->span_length), pvl->span_length, content_size) != 0) {
                    return 1;
                }
            }
        }
        pvl_clear_spans(pvl);
    }
    pvl_clear_spans(pvl);

    /*
     * Apply to mirror
     */
    if (pvl->mirror) {
        memcpy(pvl->mirror, pvl->main, pvl->length);
    }

    return 0;
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
            if (pvl->main[i] == pvl->mirror[i]) {
                /*
                 * Report diff
                 */
                pvl->leak_cb(pvl->leak_ctx, (pvl->main)+diff_start, i-diff_start);
                /*
                 * Clear trackers
                 */
                in_diff = 0;
                diff_start = 0;
            }
        } else {
            if (pvl->main[i] != pvl->mirror[i]) {
                /*
                 * Set trackers
                 */
                in_diff = 1;
                diff_start = i;
            }
        }
    }
}
