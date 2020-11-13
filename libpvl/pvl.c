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
#include "pvl.h"

typedef struct {
    char*    start;
    size_t   length;
} pvl_span;

typedef struct {
    int    partial;
    size_t change_count;
} pvl_change_header;

typedef struct {
    ptrdiff_t start;
    size_t    length;
} pvl_change;

struct pvl {
    char*          main;
    char*          mirror;
    size_t         length;
    pre_load_cb_t  pre_load_cb;
    post_load_cb_t post_load_cb;
    pre_save_cb_t  pre_save_cb;
    post_save_cb_t post_save_cb;
    leak_cb_t      leak_cb;
    int            partial;
    int            in_transaction;
    FILE*          last_save_file;
    long           last_save_pos;
    size_t         marks_index;
    size_t         marks_count;
    pvl_span     marks[];
};

size_t pvl_sizeof(size_t marks) {
    return sizeof(struct pvl)+(marks * sizeof(pvl_span));
}

static int pvl_load(struct pvl* pvl, int initial, FILE* req_from, long req_pos);
static int pvl_save(struct pvl* pvl, int partial);
static void pvl_clear_marks(struct pvl* pvl);
static void pvl_clear_memory(struct pvl* pvl);
static void pvl_coalesce_marks(struct pvl* pvl, int* coalesced_flag);
static int pvl_leak_detection(struct pvl* pvl, int partial);

struct pvl* pvl_init(char *at, size_t marks, char *main, size_t length, char *mirror,
    pre_load_cb_t pre_load_cb, post_load_cb_t post_load_cb,
    pre_save_cb_t pre_save_cb, post_save_cb_t post_save_cb,
    leak_cb_t leak_cb) {
    // Check for alignment
    size_t alignment = ((uintptr_t) at) % alignof(max_align_t);
    if (alignment != 0) {
        return NULL; // Invalid alignment
    }

    if ((!mirror) && leak_cb) {
        // Cannot have leak detection without a mirror
        return NULL;
    }

    struct pvl* pvl = (struct pvl*) at;

    // Ensure we don't have garbage. The custom sizeof function will
    // account for the flexible array at the end of the structure,
    // so there is no need to call pvl_clear_marks.
    memset(pvl, 0, pvl_sizeof(marks));

    if (marks == 0) {
        return NULL;
    }
    pvl->marks_count = marks;

    if (main == NULL) {
        return NULL;
    }
    pvl->main = main;

    if (length == 0) {
        return NULL;
    }
    pvl->length = length;

    if (mirror != NULL) {
        // Check for main/mirror overlap
        if (((mirror <= main) && ((mirror+length) > main)) ||
            ((main <= mirror) && ((main+length) > mirror))) {
            return NULL;
        }
        pvl->mirror = mirror;
    }

    // Clear the main memory
    pvl_clear_memory(pvl);

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

int pvl_begin(struct pvl* pvl) {
    if (pvl == NULL) {
        return 1;
    }

	if (pvl->in_transaction) {
		return 1;
	}

	pvl->in_transaction = 1;
	return 0;
}

int pvl_mark(struct pvl* pvl, char* start, size_t length) {
    if (pvl == NULL) {
        return 1;
    }

    if (!pvl->in_transaction) {
        return 1;
    }

    // Validate mark
    if ((length == 0) || (start < pvl->main)
            || ((start+length) > (pvl->main+pvl->length))) {
        return 1;
    }

    // TODO if current length of all marks plus the new mark
    // is over the partial write treshold - perform a partial write first,
    // then clean the marks and add the new one

    // Add it to the marks list if there is an available slot
    if (pvl->marks_index < pvl->marks_count) {
        pvl->marks[pvl->marks_index].start = start;
        pvl->marks[pvl->marks_index].length = length;
        pvl->marks_index++;
    } else {
        // Marks are full, need to coalesce them
        int coalesced = 0;
        pvl_coalesce_marks(pvl, &coalesced);
        if (coalesced) {
            // Recurse - this add will pass since at least one mark was coalesced
            return pvl_mark(pvl, start, length);
        }

        // Save a partial change
        int partial = pvl_save(pvl, 1);
        if (partial) {
            // Failed to save the partial change
            return 1;
        }

        // Clear out all marks
        pvl_clear_marks(pvl);

        // Recurse - this will pass as there are no marks
        return pvl_mark(pvl, start, length);
    }
    return 0;
}

int pvl_commit(struct pvl* pvl) {
    if (pvl == NULL) {
        return 1;
    }

    if (!pvl->in_transaction) {
        return 1;
    }

    // Commit is just a non-partial save
    pvl->in_transaction = 0;
    return pvl_save(pvl, 0);
}

int pvl_rollback(struct pvl* pvl) {
    if (pvl == NULL) {
        return 1;
    }

    if (!pvl->in_transaction) {
        return 1;
    }

    if (pvl->mirror != NULL) {
        if (pvl->partial) {
            // Perform a full copy
            memcpy(pvl->main, pvl->mirror, pvl->length);
            pvl->partial = 0;
            pvl_clear_marks(pvl);
            return 0;
        } // else
        for (size_t i = 0; i < pvl->marks_index; i++) {
            ptrdiff_t offset = pvl->marks[i].start - pvl->main;
            memcpy(pvl->marks[i].start, pvl->mirror+offset, pvl->marks[i].length);
            pvl->marks[i] = (pvl_span){0};
        }
        pvl->marks_index = 0;
        return 0;
    } // else
    pvl_clear_marks(pvl);
    pvl->in_transaction = 0;
    pvl_clear_memory(pvl);

    return pvl_load(pvl, 1, pvl->last_save_file, pvl->last_save_pos);
}

static void pvl_clear_memory(struct pvl* pvl) {
    memset(pvl->main, 0, pvl->length);
    if (pvl->mirror) {
        memset(pvl->mirror, 0, pvl->length);
    }
}

static void pvl_clear_marks(struct pvl* pvl) {
    memset(pvl->marks, 0, pvl->marks_count * sizeof(pvl_span));
    pvl->marks_index = 0;
}

static int pvl_mark_compare(const void *a, const void *b) {
    pvl_span* mark_a = (pvl_span*)a;
    pvl_span* mark_b = (pvl_span*)b;
    if (mark_a->start > mark_b->start) {
        return 1;
    }
    if (mark_a->start < mark_b->start) {
        return -1;
    }
    // Equal start locations
    return 0;
}

static void pvl_coalesce_marks(struct pvl* pvl, int* coalesced_flag) {
    int coalesced = 0;
    qsort(pvl->marks, pvl->marks_index, sizeof(pvl_span), pvl_mark_compare);
    for (size_t i = 0; i < pvl->marks_index;) {
        size_t j = i+1;
        for (; j < pvl->marks_index; j++) {
            if ((pvl->marks[i].start + pvl->marks[i].length) >= pvl->marks[j].start) {
                // Coalesce overlapping or continuous marks
                pvl->marks[i].length = pvl->marks[j].length;
                // Clear the next mark
                pvl->marks[j] = (pvl_span){0};
                // Raise the flag
                coalesced = 1;
            } else {
                break;
            }
        }
        i = j; // Advance
    }
    if (coalesced) {
        // Put the cleared marks slots at the end
        for(size_t i = 0; i < pvl->marks_index; i++) {
            if (pvl->marks[i].start) {
                continue;
            }
            pvl->marks_index = i;
            for (size_t j = i+1; j < pvl->marks_count; j++) {
                if (! pvl->marks[j].start) {
                    continue;
                }
                pvl->marks[i] = pvl->marks[j];
                pvl->marks[j] = (pvl_span){0};
                pvl->marks_index = j;
            }
        }
    }
    if (coalesced_flag != NULL) {
        *coalesced_flag = coalesced;
    }
}

/*
 * Save the currently-marked memory content
 */
static int pvl_save(struct pvl* pvl, int partial) {
    /*
     * Commit may still be called with no marks
     */
    if (! pvl->marks_index) {
        return 0;
    }

    /*
     * Early return if there is no save handler
     */
    if (pvl->pre_save_cb == NULL) {
        return 0;
    }

    /*
     * Merge overlapping and/or continuous marks
     */
    pvl_coalesce_marks(pvl, NULL);

    /*
     * Calculate the total size of the change
     */
    size_t total = sizeof(pvl_change_header);
    for (size_t i = 0; i < pvl->marks_index; i++) {
        total += sizeof(pvl_change);
        total += pvl->marks[i].length;
    }

    /*
     * Determine whether this is a full change
     */
    int full = (total == pvl->length);

    /*
     * Get the save destination through the pre-save callback
     */
    FILE* file = NULL;
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
    pvl_change_header change_header = {0};
    change_header.partial = partial;
    change_header.change_count = pvl->marks_index;
    if (fwrite(&change_header, sizeof(pvl_change_header), 1, file) != 1) {
        (pvl->post_save_cb)(pvl, full, total, file, 1);
        return 1;
    }

    /*
     * Construct and save each change
     */
    for (size_t i = 0; i < pvl->marks_index; i++) {
        pvl_change change = {0};
        change.start = pvl->marks[i].start - pvl->main;
        change.length = pvl->marks[i].length;
        if (fwrite(&change, sizeof(pvl_change), 1, file) != 1) {
            (pvl->post_save_cb)(pvl, full, total, file, 1);
            return 1;
        }
        if (fwrite(pvl->marks[i].start, pvl->marks[i].length, 1, file) != 1) {
            (pvl->post_save_cb)(pvl, full, total, file, 1);
            return 1;
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
     *
     * pvl not partial, flag not partial - apply changes
     * pvl not partial, flag partial - set pvl to partial, do not apply
     * pvl partial, flag partial - do not apply
     * pvl partial, flag not partial - clear pvl, apply entire block
     */
    if (pvl->mirror) {
        if (partial) {
            pvl->partial = partial;
        } else {
            if (pvl->partial) {
                pvl->partial = 0;
                // Perform a full copy
                memcpy(pvl->mirror, pvl->main, pvl->length);
            } else {
                for (size_t i = 0; i < pvl->marks_index; i++) {
                    ptrdiff_t offset = pvl->marks[i].start - pvl->main;
                    memcpy(pvl->mirror+offset, pvl->marks[i].start, pvl->marks[i].length);
                }
            }
        }
    }

    // Store the last complete save info in the pvl
    if (!partial) {
        pvl->last_save_file = file;
        pvl->last_save_pos = ftell(file);
    }

    /*
     * Signal that the save is done
     */
    (pvl->post_save_cb)(pvl, full, total, file, 0);

    /*
     * Perform leak detection if prereqs are met.
     *
     * Leak detection is still possible with partial writes,
     * although to avoid false positives a change should be
     * marked as soon as it is made in memory, which may
     * incur some performance overhead. Nevertheless,
     * leak detection is primarily a debugging tool that
     * can be useful even if it has certain deficiencies.
     */
    if (pvl->leak_cb) {
        pvl_leak_detection(pvl, partial);
    }

    return 0;
}

static int pvl_load(struct pvl* pvl, int initial, FILE* up_to_src, long up_to_pos) {
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
        pvl_change_header change_header = {0};
        last_good_pos = ftell(file);

        if (fread(&change_header, sizeof(pvl_change_header), 1, file) != 1) {
            // Couldn't load the change, signal the callback
            if ((pvl->post_load_cb)(pvl, file, 1, last_good_pos)) {
                reset_load = 1;
                goto read_loop;
            }
            return 1;
        }

        /*
         * Read each mark header and content
         */
        for (size_t i = 0; i < change_header.change_count; i++) {
            pvl_change change = {0};
            if (fread(&change, sizeof(pvl_change), 1, file) != 1) {
                // Couldn't load the change, signal the callback
                if ((pvl->post_load_cb)(pvl, file, 1, last_good_pos)) {
                    reset_load = 1;
                    goto read_loop;
                }
                return 1;
            }
            if (fread(pvl->main+change.start, change.length, 1, file) != 1) {
                // Couldn't load the change, signal the callback
                if ((pvl->post_load_cb)(pvl, file, 1, last_good_pos)) {
                    reset_load = 1;
                    goto read_loop;
                }
                return 1;
            }
        }

        last_good_pos = ftell(file);

        /*
         * Apply it to the mirror
         *
         * Do not apply partial changes as the load may fail later on
         */
        if (pvl->mirror && !change_header.partial) {
            memcpy(pvl->mirror, pvl->main, pvl->length);
        }

        // Report success  via the post-load callback
        if ((pvl->post_load_cb)(pvl, file, 0, last_good_pos)) {
            goto read_loop;
        }
        return 0;
    }
}

static int pvl_leak_detection(struct pvl* pvl, int partial) {
    // TODO - don't wait for mirror application, work with marks
    (void)(pvl);
    (void)(partial);
    size_t in_diff = 0;
    size_t start = 0;
    for (size_t i = 0; i <= pvl->length; i++) {
        if (in_diff) {
            if (pvl->main[i] == pvl->mirror[i]) { //diff over
                // report diff
                (pvl->leak_cb)(pvl, (pvl->main)+start, i-start, partial);
                // clear trackers
                in_diff = 0;
                start = 0;
            } // else do nothing
        } else {
            if (pvl->main[i] != pvl->mirror[i]) { //diff starts
                // set trackers
                in_diff = 1;
                start = i;
            } // else do nothing
        }
    }
    return 0;
}
