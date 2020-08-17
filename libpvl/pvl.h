/*
 * Copyright 2020 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * libpvl - a system prevalence library (header)
 *
 * System prevalence is a persistence technique where objects and
 * changes to them are stored using some serialization in a journal.
 *
 * Restoring the state is achieved through reading and replaying
 * all changes in the journal.
 *
 * Using C one can take a pragmatic approach and store the actual
 * memory representation without performing serialization.
 *
 * This direct access allows to changes to be tracked after they
 * are made instead of wrapping them in transaction objects, as
 * most high-level language system prevalence libraries require.
 *
 * This library exposes a minimal interface where all options are
 * set upon initialization. Ops concerns are handled via callbacks
 * so that calling code can remain simple.
 */
#pragma once

#include <stddef.h>
#include <stdio.h>

/*
 * Define an opaque type ..
 */
typedef struct pvl_s pvl_t;

/*
 * .. that a caller can allocate.
 *
 * Pass the number of marks that you want to keep.
 */
size_t pvl_sizeof_pvl_t(size_t marks);

/*
 * Callback for preparing to load changes
 *
 * Passed parameters
 * - Initial flag (for initial loading and rollback when there is no mirror)
 * - Pointer to a FILE* - set to load changes from it using fread
 *
 * Returns
 * - Standard int behavior
 */
typedef int (*pre_load_cb_t)(pvl_t* pvl, int initial, FILE** file, int* repeat);

/*
 * Callback for acknowledging loaded changes
 *
 * TODO
 * - Pointer to a repeat flag - set if pvl should call the callback again
 *   for additional changes
 */
typedef int (*post_load_cb_t)(pvl_t* pvl, FILE* file, int status, long fpos, int* repeat);

/*
 * Callback for preparing to persist changes
 *
 * Passed parameters
 * - Length of the current change to be persisted
 * - Pointer to an FILE* - set to to save changes to it using fwrite
 *
 * Returns
 * - Standard int behavior
 */
typedef int (*pre_save_cb_t)(pvl_t* pvl, size_t length, FILE** file);

/*
 * Callback for acknowledging persisted changes.
 *
 * Use to perform post-save actions, like close an fd or send
 * the persisted buffer over the network.
 *
 * Passed parameters
 * - The length of the content that was written
 * - The FILE * that the content was written to
 *
 * Returns
 * - Standard int behavior
 */
typedef int (*post_save_cb_t)(pvl_t* pvl, size_t length, FILE* file);

/*
 * Callback for reporting leaks
 *
 * Passed parameters
 * - Start location of the leaked area
 * - Length of the leaked area
 * - Partial detection flag - leaks reported with the partial flag on may be
 *                            false positives - not yet marked.
 *
 * Returns
 * - Nothing
 */
typedef void (*leak_cb_t)(pvl_t* pvl, void* start, size_t length, int partial);

/*
 * Initialize pvl_t at the provided location.
 *
 * Ensure that it is aligned to max_align_t. Do not copy after initialization.
 *
 * at                 - Pointer to where the pvl object should be initialized
 * marks              - Number of marks to store.
 * main               - Pointer to the start of a pvl-managed memory block
 * length             - The lenght of the pvl-managed memory block
 * mirror             - Pointer to the start of a pvl-managed mirror memory block
 *                      that is used for additional capabilities like
 *                      leak detection and fast rollback.
 *                      Set to NULL to disable the additional capabilities.
 * pre_load_cb        - Callback for loading changes. It will be called immediately if set.
 * post_load_cb       - Callback to acknowledge loaded changes
 * pre_save_cb        - Callback used to prepare for persisting changes
 * post_save__cb      - Callback used to acknowledge saved changed
 * leak_cb            - Callback used to report detected leaks - changed but
 *                      not marked memory spans after pvl_commit
 */
pvl_t* pvl_init(char *at, size_t marks,
    char *main, size_t length, char *mirror,
    pre_load_cb_t pre_load_cb, post_load_cb_t post_load_cb,
    pre_save_cb_t pre_save_cb, post_save_cb_t post_save_cb,
    leak_cb_t leak_cb);

/*
 * Mark a span of memory for inclusion in the next commit.
 *
 * Make sure to call mark after the memory has been updated
 * as marks can trigger partial writes before commit is called.
 */
int pvl_mark(pvl_t* pvl, char* start, size_t length);

/*
 * Persist the marked spans and clear marks.
 */
int pvl_commit(pvl_t* pvl);

/*
 * Rollback any marked changes. If a mirror is available it will
 * be used to rollback the changes. Otherwise the load callbacks will
 * be called with the initial flag raised.
 */
int pvl_rollback(pvl_t* pvl);
