/*
 * Copyright 2020 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * libpvl - a system prevalence library (implementation)
 */

#ifndef WARNING_DO_NOT_INCLUDE_PLV_C
#error "Do not include pvl.c. Use the header and link to the shared object."
#endif

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pvl.h"

typedef struct {
    char*    start;
    size_t   length;
} pvl_mark_t;

typedef struct {
    size_t id;
    time_t time;
    int    partial;
    size_t change_count;
} pvl_change_header_t;

typedef struct {
    // start
    size_t length;
} pvl_change_t;

struct pvl_s {
    char*          main;
    char*          mirror;
    size_t         length;
    pre_load_cb_t  pre_load_cb;
    post_load_cb_t post_load_cb;
    pre_save_cb_t  pre_save_cb;
    post_save_cb_t post_save_cb;
    leak_cb_t      leak_cb;
    int            partial;
    size_t         change_id;
    size_t         marks_index;
    size_t         marks_count;
    pvl_mark_t     marks[];
};

size_t pvl_sizeof_pvl_t(size_t marks) {
    return sizeof(struct pvl_s)+(marks * sizeof(pvl_mark_t));
}

static int pvl_load(pvl_t* pvl, int initial);
static int pvl_save(pvl_t* pvl, int partial);
static void pvl_clear_marks(pvl_t* pvl);
static void pvl_coalesce_marks(pvl_t* pvl, int* coalesced_flag);
static int pvl_leak_detection(pvl_t* pvl);

pvl_t* pvl_init(char *at, size_t marks, char *main, size_t length, char *mirror,
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

    // Ensure we don't have garbage
    memset(at, 0, pvl_sizeof_pvl_t(marks));

    pvl_t* pvl = (pvl_t*) at;

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

    pvl->post_load_cb = post_load_cb;

    // Perform initial loading if a load callback is available
    if (pre_load_cb != NULL) {
        pvl->pre_load_cb = pre_load_cb;
        int load_result = pvl_load(pvl, 1);
        // TODO handle
        if (load_result != 0) {
            return NULL;
        }
    }

    // A load-only (e.g. replicating) pvl might have no save callback.
    pvl->pre_save_cb = pre_save_cb;
    pvl->post_save_cb = post_save_cb;

    // The leak detection is optional and will be checked upon use.
    pvl->leak_cb = leak_cb;

    return pvl;
}

int pvl_mark(pvl_t* pvl, char* start, size_t length) {
    if (pvl == NULL) {
        return 1;
    }

    // Validate mark
    if ((start == NULL) || (length == 0) || (start < pvl->main)
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
        // Different approaches are possible here - take a lazy one
        // and improve later if needed. It is best to not do it at all.
        int coalesced = 0;
        pvl_coalesce_marks(pvl, &coalesced);
        if (coalesced) {
            // Recurse - this add will pass since at least one mark was coalesced
            return pvl_mark(pvl, start, length);
        }

        if (pvl->leak_cb != NULL) {
            // Leak detection is enabled and it cannot be performed with partial writes
            return 1;
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

int pvl_commit(pvl_t* pvl) {
    if (pvl == NULL) {
        return 1;
    }

    // Commit is just a non-partial save
    return pvl_save(pvl, 0);
}

int pvl_rollback(pvl_t* pvl) {
    if (pvl == NULL) {
        return 1;
    }
    if (pvl->mirror != NULL) {
        if (pvl->partial) {
            // TODO
            // knowledge of previous marks is lost - need to store a way that reverts them
            // perhaps trigger a full snapshot store after restore from the mirror ?
            //
            // - or -
            //
            // do nothing - let the load code handle it, as it has to handle this case anyway
            memcpy(pvl->main, pvl->mirror, pvl->length);
            pvl->partial = 0;
            pvl_clear_marks(pvl);
            return 0;
        } // else
        for (size_t i; i < pvl->marks_index; i++) {
            // TODO for each mark - memcpy, clear mark
        }
        // TODO reset mark index
        return 0;
    } // else
    pvl_clear_marks(pvl);
    return pvl_load(pvl, 1);
}

int pvl_replay(pvl_t* pvl) {
    if (pvl == NULL) {
        return 1;
    }

    // Replay is a non-initial load
    return pvl_load(pvl, 0);
}

static int pvl_save(pvl_t* pvl, int partial) {
    if (pvl->leak_cb && pvl->mirror) {
        pvl_leak_detection(pvl);
    }

    // Commit may still be called with no marks
    if (! pvl->marks_index) {
        return 0;
    }

    // Early return if there is no save handler
    if (pvl->pre_save_cb == NULL) {
        pvl_clear_marks(pvl);
        return 0;
    }

    pvl_coalesce_marks(pvl, NULL);

    // Calculate the total size
    size_t total = sizeof(pvl_change_header_t);
    for (size_t i = 0; i < pvl->marks_index; i++) {
        if (pvl->marks[i].start == NULL) {
            break;
        }
        total += sizeof(pvl_change_t);
        total += pvl->marks[i].length;
    }

    FILE* file = NULL;

    // Call save callback
    (*pvl->pre_save_cb)(total, &file);

    // Fail if the callback did not provide a place to save
    if (file == NULL) {
        return 1;
    }

    size_t write_count = 0;

    // Construct and save the header
    pvl_change_header_t change_header = {0};
    change_header.id = pvl->change_id;
    change_header.time = time(NULL);
    change_header.partial = partial;
    change_header.change_count = pvl->marks_index;
    write_count = fwrite(&change_header, sizeof(pvl_change_header_t), 1, file);
    if (write_count != 1) {
        return 1;
    }

    // Construct and save each change
    for (size_t i = 0; i < pvl->marks_index; i++) {
        pvl_change_t change = {0};
        write_count = fwrite(&change, sizeof(pvl_change_t), 1, file);
        if (write_count != 1) {
            return 1;
        }
        write_count = fwrite(pvl->marks[i].start, pvl->marks[i].length, 1, file);
        if (write_count != 1) {
            return 1;
        }
    }

    // leak detection
    // apply to mirror
    // if partial - don't apply to mirror but store a flag
    // and re-image everything once the final save is done
    // otherwise rollback won't be available

    // Signal that the save is done
    if (pvl->post_save_cb != NULL) {
        int signal_result = pvl->post_save_cb(total, file);
        if (signal_result) {
            return signal_result;
        }
    }

    return 0;
}

static int pvl_load(pvl_t* pvl, int initial) {
    /*
     *  Probably the most important issue at hand
     * is how to recover from a failed or incomplete load.
     * This is easy if a mirror is in use - just revert from it.
     *
     * However, if there is no mirror you have to reload
     * all changes up to the current one - and then signal back
     * to the application that the change won't do.
     *
     * If this is not okay perhaps some sort of a non-apply change
     * saved would allow for a easier revert ?
     *
     * Or even allocate memory during change loading and free it
     * afterwards - the horror :)
     */
    return 0;
}

static void pvl_clear_marks(pvl_t* pvl) {
    // OPT a memset might be faster :)
    for (size_t i = 0; i < pvl->marks_count; i++) {
        pvl->marks[i].start = NULL;
        pvl->marks[i].length = 0;
    }
    pvl->marks_index = 0;
}

static int pvl_mark_compare(const void *a, const void *b) {
    pvl_mark_t* mark_a = (pvl_mark_t*)a;
    pvl_mark_t* mark_b = (pvl_mark_t*)b;
    // Be thorough - arithmetic operations on NULL are undefined,
    // pointer arithmetic can overflow as pointers are unsigned.
    if ((!mark_a->start) && (!mark_b->start)) {
        return 0;
    }
    if ((!mark_a->start) && (mark_b->start)) {
        return -1;
    }
    if ((mark_a->start) && (!mark_b->start)) {
        return 1;
    }
    if (mark_a->start > mark_b->start) {
        return 1;
    }
    if (mark_a->start < mark_b->start) {
        return -1;
    }
    // Equal start locations
    return 0;
}

static int pvl_mark_reverse_compare(const void *a, const void *b) {
    return pvl_mark_compare(b, a);
}

static void pvl_coalesce_marks(pvl_t* pvl, int* coalesced_flag) {
    // this function should leave the marks sorted
    // therefore a more appropriate name is in order
    // probably something like pvl_tidy_marks or
    // pvl_optimize_marks, pvl_organize_marks, etc
    // tbd :)
    int coalesced = 0;
    qsort(pvl->marks, pvl->marks_index, sizeof(pvl_mark_t), pvl_mark_compare);
    for (size_t i = 0; i < pvl->marks_index;) {
        size_t j = i+1;
        for (; j < pvl->marks_index; j++) {
            if ((pvl->marks[i].start + pvl->marks[i].length) >= pvl->marks[j].start) {
                // Coalesce overlapping or continuous marks
                pvl->marks[i].length = pvl->marks[j].length;
                // Clear the next mark
                pvl->marks[j].start = NULL;
                pvl->marks[j].length = 0;
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
        // OPT coalescing marks leaves the list sorted - shift empty marks is ~ linear
        qsort(pvl->marks, pvl->marks_index, sizeof(pvl_mark_t), pvl_mark_reverse_compare);
        for (size_t i = 0; i < pvl->marks_index; i++) {
            if (pvl->marks[i].start == NULL) {
                // Find the first one
                pvl->marks_index = i+1;
                break;
            }
        }
    }
    if (coalesced_flag != NULL) {
        *coalesced_flag = coalesced;
    }
}

static int pvl_leak_detection(pvl_t* pvl) {
    // TODO :)
    return 0;
}
