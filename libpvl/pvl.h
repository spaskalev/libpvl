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
 * Define an incomplete type
 */
struct pvl;

/*
 * .. that a caller can allocate.
 *
 * span_count must match with pvl_init
 */
size_t pvl_sizeof(size_t span_count);

/*
 * Callback for persisting changes
 *
 * Passed parameters
 * - Caller-provided context
 * - Address to read change content from
 * - Length of change content to be stored
 * - Number of remaining bytes till the entire change is passed to the callback.
 *    It will be set to zero when writing the last chunk.
 *    This can be used by callback handlers to wrap entire changes with additional
 *    features, e.g. checksum, compression, encryption, signing, etc.
 *
 * Returns
 * - Nothing
 */
typedef int write_callback(void *ctx, void *from, size_t length, size_t remaining);

/*
 * Callback for retrieving changes
 *
 * Passed parameters
 * - Caller-provided context
 * - Address to write change content to
 * - Length of change content to be read
 *    This can be set to zero - in this case libpvl is querying the
 *    remaining bytes availability
 * - Number of remaining bytes to be read as part of the same change.
 *    Callbacks can use this hint to verify additional features as documented
 *    in the write callback documentation.
 *    The read callback should not return an error during continuous change
 *    read with decreasing remaning bytes.
 *
 * Returns
 * - Nothing
 */
typedef int read_callback(void *ctx, void *to, size_t length, size_t remaining);

/*
 * Callback for reporting leaks
 *
 * Passed parameters
 * - Caller-provided context
 * - Address of the detected leak
 * - Length of the detected leak
 *
 * Returns
 * - Nothing
 */
typedef void leak_callback(void *ctx, void *start, size_t length);

/*
 * Initialize pvl_t at the provided location.
 *
 * Ensure that it is aligned to max_align_t. Do not copy after initialization.
 *
 * Passed parameters
 * at                 - Pointer to where the pvl object should be initialized
 * main               - Pointer to the start of a pvl-managed memory block
 * length             - The lenght of the pvl-managed memory block
 *                      It must be divisible by span_count.
 * span_count         - The number of internal spans used to track changes
 *
 * Returns
 * pvl_t*             - A valid pointer to pvl_t in the case of a successful initialization,
 *                      NULL otherwise.
 */
struct pvl *pvl_init(char *at, char *main, size_t length, size_t span_count);

/* Configure the read handler on a pvl instance and trigger a load */
int pvl_set_read_cb(struct pvl *pvl, void *read_ctx, read_callback read_cb);

/* Configure the write handler on a pvl instance */
int pvl_set_write_cb(struct pvl *pvl, void *write_ctx, write_callback write_cb);

/* Configure a mirror on the pvl instance, used for leak detection and other stats*/
int pvl_set_mirror(struct pvl *pvl, char *mirror);

/* Configure the leak detection handler on a pvl instance. Requires a mirror */
int pvl_set_leak_cb(struct pvl *pvl, void *leak_ctx, leak_callback leak_cb);

/*
 * Mark a span of memory for inclusion in the next commit.
 */
int pvl_mark(struct pvl *pvl, const char *start, size_t length);

/*
 * Persist the marked spans.
 */
int pvl_commit(struct pvl *pvl);
