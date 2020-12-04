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
    char   *start;
    size_t length;
} mark;

typedef struct {
    size_t change_count;
} change_header;

typedef struct {
    ptrdiff_t start;
    size_t    length;
} mark_header;

struct pvl {
    char               *main;
    char               *mirror;
    size_t             length;
    pre_load_callback  *pre_load_cb;
    post_load_callback *post_load_cb;
    pre_save_callback  *pre_save_cb;
    post_save_callback *post_save_cb;
    leak_callback      *leak_cb;
    int                in_transaction;
    FILE               *last_save_file;
    int                marks_combined;
    long               last_save_pos;
    size_t             marks_index;
    size_t             marks_count;
    mark               marks[];
};

size_t pvl_sizeof(size_t marks) {
    return sizeof(struct pvl)+(marks * sizeof(mark));
}

static int pvl_load(struct pvl *pvl, int initial, FILE *req_from, long req_pos);
static int pvl_save(struct pvl *pvl);
//static void pvl_clear_marks(struct pvl *pvl);
static void pvl_clear_memory(struct pvl *pvl);
static int pvl_coalesce_marks(struct pvl *pvl);
static void pvl_detect_leaks(struct pvl *pvl);
static void pvl_detect_leaks_inner(struct pvl *pvl, size_t from, size_t to);

struct pvl *pvl_init(char *at, size_t marks, char *main, size_t length, char *mirror,
    pre_load_callback pre_load_cb, post_load_callback post_load_cb,
    pre_save_callback pre_save_cb, post_save_callback post_save_cb,
    leak_callback leak_cb) {
    // Check for alignment
    size_t alignment = ((uintptr_t) at) % alignof(max_align_t);
    if (alignment != 0) {
        return NULL; // Invalid alignment
    }

    if ((!mirror) && leak_cb) {
        // Cannot have leak detection without a mirror
        return NULL;
    }

    struct pvl *pvl = (struct pvl*) at;

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

int pvl_begin(struct pvl *pvl) {
    if (pvl == NULL) {
        return 1;
    }

    if (pvl->in_transaction) {
        return 1;
    }

    pvl->in_transaction = 1;
    return 0;
}

int pvl_mark(struct pvl *pvl, char *start, size_t length) {
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

    // Add it to the marks list if there is an available slot
    if (pvl->marks_index < pvl->marks_count) {
        pvl->marks[pvl->marks_index].start = start;
        pvl->marks[pvl->marks_index].length = length;
        pvl->marks_index++;
    } else {
		// TODO add a flag to prevent constant coalescing
        // Mark slots are full, need to coalesce them
		if (pvl_coalesce_marks(pvl)) {
			// Recurse - this add will pass since at least one mark was coalesced
			return pvl_mark(pvl, start, length);
		}

        // Marks cannot be coalesced, need to be combined

        // No action required if new mark is contained or contains a current mark
        // Use marks_count as the sentinel value to avoid size_t underflow in
        // left-of/right-of calculations
        size_t closest_next = pvl->marks_count;
        size_t closest_prev = pvl->marks_count;
        for (size_t i = 0; i < pvl->marks_index; i++) {
			if (start >= pvl->marks[i].start && length <= pvl->marks[i].length) {
				// New mark is already contained, nothing more to do
				return 0;
			}
			if (start <= pvl->marks[i].start && length >= pvl->marks[i].length) {
				// New mark contains current mark, swap
				pvl->marks[i].start = start;
				pvl->marks[i].length = length;
				return 0;
			}
			if (length > pvl->marks[i].length) { // right-of
				if (closest_prev == pvl->marks_count) {
					closest_prev = i;
				} else {
					if ((length - pvl->marks[i].length) < (length - pvl->marks[closest_prev].length)) {
						closest_prev = i;
					}
				}
			}
			if (start < pvl->marks[i].start) { // left of
				if (closest_next == pvl->marks_count) {
					closest_next = i;
				} else {
					if ((pvl->marks[i].start - start) < (pvl->marks[closest_next].start - start)) {
						closest_next = i;
					}
				}
			}
		}
		
		// At this point either one or both are set
		if ((closest_next != pvl->marks_count) && (closest_prev != pvl->marks_count)) {
			size_t next_distance = pvl->marks[closest_next].start - start;
			size_t prev_distance = length - pvl->marks[closest_prev].length;
			if (next_distance <= prev_distance) {
				// use next
				closest_prev = pvl->marks_count;
			} else {
				// use prev
				closest_next = pvl->marks_count;
			}
		}
		
		// if mark is not contained - extend closest mark
		if (closest_next != pvl->marks_count) {
			pvl->marks[closest_next].start = start;
		} else {
			pvl->marks[closest_prev].length = length;
		}

		pvl->marks_combined = 1;
        return 0;
    }
    return 0;
}

int pvl_commit(struct pvl *pvl) {
    if (pvl == NULL) {
        return 1;
    }

    /*
     * Commit may not be called outside of a transaction
     */
    if (!pvl->in_transaction) {
        return 1;
    }

    /*
     * Commit may not be called without marks
     */
    if (! pvl->marks_index) {
        return 1;
    }

    pvl->in_transaction = 0;
    pvl->marks_combined = 0;
    return pvl_save(pvl);
}

static void pvl_clear_memory(struct pvl *pvl) {
    memset(pvl->main, 0, pvl->length);
    if (pvl->mirror) {
        memset(pvl->mirror, 0, pvl->length);
    }
}

//static void pvl_clear_marks(struct pvl *pvl) {
//    memset(pvl->marks, 0, pvl->marks_count * sizeof(mark));
//    pvl->marks_index = 0;
//}

static int pvl_mark_compare(const void *a, const void *b) {
    mark *mark_a = (mark*)a;
    mark *mark_b = (mark*)b;
    if (mark_a->start > mark_b->start) {
        return 1;
    }
    if (mark_a->start < mark_b->start) {
        return -1;
    }
    // Equal start locations
    return 0;
}

static int pvl_coalesce_marks(struct pvl *pvl) {
    int coalesced = 0;
    qsort(pvl->marks, pvl->marks_index, sizeof(mark), pvl_mark_compare);
    for (size_t i = 0; i < pvl->marks_index;) {
        size_t j = i+1;
        for (; j < pvl->marks_index; j++) {
            if ((pvl->marks[i].start + pvl->marks[i].length) >= pvl->marks[j].start) {
                // Coalesce overlapping or continuous marks
                pvl->marks[i].length = pvl->marks[j].length;
                // Clear the next mark
                pvl->marks[j] = (mark){0};
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
                pvl->marks[j] = (mark){0};
                pvl->marks_index = j;
            }
        }
    }
    return coalesced;
}

/*
 * Save the currently-marked memory content
 */
static int pvl_save(struct pvl *pvl) {
    /*
     * Merge overlapping and/or continuous marks
     */
    pvl_coalesce_marks(pvl);

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
    size_t total = sizeof(change_header);
    for (size_t i = 0; i < pvl->marks_index; i++) {
        total += sizeof(mark_header);
        total += pvl->marks[i].length;
    }

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
    change_header change_header = {0};
    change_header.change_count = pvl->marks_index;
    if (fwrite(&change_header, sizeof(change_header), 1, file) != 1) {
        (pvl->post_save_cb)(pvl, full, total, file, 1);
        return 1;
    }

    /*
     * Construct and save each change
     */
    for (size_t i = 0; i < pvl->marks_index; i++) {
        mark_header change = {0};
        change.start = pvl->marks[i].start - pvl->main;
        change.length = pvl->marks[i].length;
        if (fwrite(&change, sizeof(mark_header), 1, file) != 1) {
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
     */
    if (pvl->mirror) {
		for (size_t i = 0; i < pvl->marks_index; i++) {
			ptrdiff_t offset = pvl->marks[i].start - pvl->main;
			memcpy(pvl->mirror+offset, pvl->marks[i].start, pvl->marks[i].length);
		}
    }

    /*
     * Signal that the save is done
     */
    (pvl->post_save_cb)(pvl, full, total, file, 0);

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
        change_header change_header = {0};
        last_good_pos = ftell(file);

        if (fread(&change_header, sizeof(change_header), 1, file) != 1) {
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
            mark_header change = {0};
            if (fread(&change, sizeof(mark_header), 1, file) != 1) {
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
         * Apply it to the mirror.
         * TODO - optimize this only for changed spans
         */
        if (pvl->mirror) {
            memcpy(pvl->mirror, pvl->main, pvl->length);
        }

        // Report success  via the post-load callback
        if ((pvl->post_load_cb)(pvl, file, 0, last_good_pos)) {
            goto read_loop;
        }
        return 0;
    }
}

static void pvl_detect_leaks(struct pvl *pvl) {
    size_t previous = 0;
    for (size_t i = 0; i < pvl->marks_index; i++) {
        mark m = pvl->marks[i];
        pvl_detect_leaks_inner(pvl, previous, m.start - pvl->main);
        previous = m.start - pvl->main + m.length;
    }
    pvl_detect_leaks_inner(pvl, previous, pvl->length);
}

static void pvl_detect_leaks_inner(struct pvl *pvl, size_t from, size_t to) {
    size_t in_diff = 0;
    size_t diff_start = 0;
    for (size_t i = from; i <= to; i++) {
        if (in_diff) {
            if (pvl->main[i] == pvl->mirror[i]) { //diff over
                // report diff
                // toggle probable leak detection on combined marks
                (pvl->leak_cb)(pvl, (pvl->main)+diff_start, i-diff_start, pvl->marks_combined);
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
