/*
 * Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * libpvl - a system prevalence library (implementation)
 */

#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitset.h"
#include "pvl.h"

struct pvl {
	char *main;
	char *mirror;
	size_t length;
	/* read context and callback */
	void *read_ctx;
	read_callback *read_cb;
	/* write context and callback */
	void *write_ctx;
	write_callback *write_cb;
	/* leak detection context and callback */
	void *leak_ctx;
	leak_callback *leak_cb;
	size_t span_length;
	size_t span_count;
	unsigned char spans[];
};

struct pvl_span {
	size_t index;
	size_t length;
	_Bool marked;

};

size_t pvl_sizeof(size_t span_count) {
	return sizeof(struct pvl)+(bitset_size(span_count) * sizeof(char));
}

static size_t pvl_next_span(struct pvl *pvl, size_t from, struct pvl_span *span);
static void pvl_clear_span(struct pvl *pvl, struct pvl_span span);
static void pvl_stat(struct pvl *pvl, size_t *spans, size_t *size);
static int pvl_load(struct pvl *pvl);
static int pvl_save(struct pvl *pvl);
static void pvl_detect_leaks(struct pvl *pvl);
static void pvl_detect_leaks_inner(struct pvl *pvl, size_t from, size_t to);

struct pvl *pvl_init(char *at, char *main, size_t length, size_t span_count) {
	/* Check for alignment */
	size_t alignment = ((uintptr_t) at) % alignof(max_align_t);
	if (alignment != 0) {
		return NULL;
	}

	/* main must not be NULL */
	if (main == NULL) {
		return NULL;
	}

	/* length must be positive */
	if (length == 0) {
		return NULL;
	}

	/* span_count must be positive */
	if (span_count == 0) {
		return NULL;
	}

	/* length must be divisible by span_count */
	if (length % span_count) {
		return NULL;
	}

	struct pvl *pvl = (struct pvl*) at;

	/* Zero the pvl destination. The custom sizeof function will
	   account for the flexible array at the end of the structure. */
	memset(pvl, 0, pvl_sizeof(span_count));

	pvl->span_count = span_count;
	pvl->main = main;
	pvl->length = length;
	pvl->span_length = pvl->length / pvl->span_count;

	return pvl;
}

int pvl_set_read_cb(struct pvl *pvl, void *read_ctx, read_callback read_cb) {
	if (pvl == NULL) {
		return 1;
	}
	if (pvl->read_cb || pvl->read_ctx) {
		return 1; /* already set */
	}
	pvl->read_ctx = read_ctx;
	pvl->read_cb = read_cb;
	return pvl_load(pvl);
}

int pvl_set_write_cb(struct pvl *pvl, void *write_ctx, write_callback write_cb) {
	if (pvl == NULL) {
		return 1;
	}
	if (pvl->write_cb || pvl->write_ctx) {
		return 1; /* already set */
	}
	pvl->write_ctx = write_ctx;
	pvl->write_cb = write_cb;
	return 0;
}

int pvl_set_mirror(struct pvl *pvl, char *mirror) {
	if (pvl == NULL) {
		return 1;
	}
	if (pvl->mirror) {
		return 1; /* already set */
	}
	if ((mirror != NULL)  &&
			(((mirror <= pvl->main) && ((mirror+pvl->length) > pvl->main)) ||
			((pvl->main <= mirror) && ((pvl->main+pvl->length) > mirror)))) {
		return 1; /* Overlap between main and mirror blocks is not allowed */
	}
	pvl->mirror = mirror;
	return 0;
}

int pvl_set_leak_cb(struct pvl *pvl, void *leak_ctx, leak_callback leak_cb){
	if (pvl == NULL) {
		return 1;
	}
	if (pvl->leak_cb || pvl->leak_ctx) {
		return 1; /* already set */
	}
	if (!pvl->mirror) {
		return 1; /* Leak detection requires a mirror */
	}
	pvl->leak_ctx = leak_ctx;
	pvl->leak_cb = leak_cb;
	return 0;
}

int pvl_mark(struct pvl *pvl, const char *start, size_t length) {
	if (pvl == NULL) {
		return 1;
	}

	/* Validate the span */
	if ((start == NULL) || (length == 0) || (start < pvl->main)
			|| ((start+length) > (pvl->main+pvl->length))) {
		return 1;
	}

	/* Set the matching spans */
	size_t from_pos = (start - pvl->main) / pvl->span_length;
	size_t to_pos = ((start+length) - pvl->main) / pvl->span_length;
	bitset_set_range(pvl->spans, from_pos, to_pos);

	return 0;
}

int pvl_commit(struct pvl *pvl) {
	if (pvl == NULL) {
		return 1;
	}

	/* Perform leak detection */
	if (pvl->leak_cb) {
		pvl_detect_leaks(pvl);
	}

	return pvl_save(pvl);
}

/* Find the next continuous span */
static size_t pvl_next_span(struct pvl *pvl, size_t from, struct pvl_span *span) {
	if (from == pvl->span_count) {
		return 0;
	}

	span->marked = bitset_test(pvl->spans, from);
	span->index = from * pvl->span_length;
	span->length = pvl->span_length;

	for (size_t i = from+1; i < pvl->span_count; i++) {
		if (span->marked ^ bitset_test(pvl->spans, i)) {
			return i;
		}
		span->length += pvl->span_length;
	}

	return pvl->span_count;
}

static void pvl_clear_span(struct pvl *pvl, struct pvl_span span) {
	size_t from_pos = span.index;
	size_t to_pos = span.index+span.length;
	bitset_clear_range(pvl->spans, from_pos, to_pos);
}

static void pvl_stat(struct pvl *pvl, size_t *spans, size_t *size) {
	*spans = 0;
	*size = 0;
	size_t next = 0;
	struct pvl_span span;
	while((next = pvl_next_span(pvl, next, &span))) {
		if (span.marked) {
			(*spans)++;
			*size += span.length;
		}
	}
}

/* Save the currently-marked memory content */
static int pvl_save(struct pvl *pvl) {

	/* Early return if there is no write callback */
	if (pvl->write_cb == NULL) {
		return 0;
	}

	/* Get the total number of spans and marked bytes */
	size_t spans = 0;
	size_t size = 0;
	pvl_stat(pvl, &spans, &size);

	/* Nothing to save */
	if (spans == 0) {
		return 0;
	}

	/* Track remaining bytes for write hints */
	size_t content_size = size + (spans * 2 * sizeof(size_t));

	/* Create a header for the change, accouting for the span header overhead */
	size_t header[2] = {0};
	header[0] = spans;
	header[1] = content_size;

	/* Construct and save the change header */
	if (pvl->write_cb(pvl->write_ctx, &header, sizeof(header), header[1])) {
		return 1;
	}

	/* Save each span */
	size_t next = 0;
	struct pvl_span span;
	while((next = pvl_next_span(pvl, next, &span))) {
		if (! span.marked) {
			continue;
		}

		/* Write the span header */
		header[0] = span.index;
		header[1] = span.index + span.length;
		content_size -= sizeof(header);
		if(pvl->write_cb(pvl->write_ctx, &header, sizeof(header), content_size)) {
			return 1;
		}

		/* Write the span content */
		content_size -= header[1];
		if(pvl->write_cb(pvl->write_ctx, pvl->main + span.index, span.length, content_size)) {
			return 1;
		}
	}

	next = 0;
	while((next = pvl_next_span(pvl, next, &span))) {
		if (! span.marked) {
			continue;
		}
		/* Apply to mirror */
		if (pvl->mirror) {
			memcpy(pvl->mirror + span.index, pvl->main + span.index, span.length);
		}
		/* Clear the span */
		pvl_clear_span(pvl, span);
	}

	return 0;
}

static int pvl_load(struct pvl *pvl) {
	while (1) {
		/* Try to read a change */
		int read_result;
		size_t header[2] = {0};

		read_result = pvl->read_cb(pvl->read_ctx, &header, sizeof(header), 0);
		if (read_result == EOF) {
			/* The read callback has successfully read all (or none) changes */
			break;
		}
		if (read_result) {
			/* The read callback failed to read a subsequent change but the pvl memory state
			   is not disrupted - operation can continue. */
			break;
		}

		size_t spans = header[0];
		size_t content_size = header[1];

		/*
		 * Validate the change header
		 */
		if (spans == 0) {
			break; /* spans cannot be zero */
		}
		if (spans > (pvl->length/2)) {
			break; /* span count upper bound for a byte-tracking pvl with every other byte marked */
		}
		if (content_size < (spans*(sizeof(header)+1))) {
			break; /* content size lower bound for a byte-tracking single byte mark */
		}
		if (content_size > (pvl->length/2*(sizeof(header)+1))) {
			break; /* content size upper bound for a byte-tracking pvl with every other byte marked */
		}

		read_result = pvl->read_cb(pvl->read_ctx, NULL, 0, content_size);
		if (read_result != 0) {
			/* The read callback indicated that it cannot serve the remaining bytes
			   but the pvl memory state is not disrupted - operation can continue. */
			 break;
		}

		/* Read each span */
		for (size_t i = 0; i < spans; i++) {
			/* At this point read should always succeed up to the remaining bytes. */

			/* Read the header */
			content_size -= sizeof(header);
			if (pvl->read_cb(pvl->read_ctx, &header, sizeof(header), content_size) != 0) {
				return 1;
			}

			/*
			 * Validate the span header
			 */
			if (header[0] >= pvl->length) {
				return 1; /* span start location must be within pvl main block */
			}
			if (header[1] >= pvl->length) {
				return 1; /* span end location must be within pvl main block */
			}
			if (header[1] <= header[0]) {
				return 1; /* invalid span end location */
			}

			/* Read the content */
			content_size -= header[1];
			if (pvl->read_cb(pvl->read_ctx, pvl->main + header[0], header[1] - header[0], content_size) != 0) {
				return 1;
			}
		}
	}

	/* Apply to mirror */
	if (pvl->mirror) {
		memcpy(pvl->mirror, pvl->main, pvl->length);
	}

	return 0;
}

static void pvl_detect_leaks(struct pvl *pvl) {
	size_t next = 0;
	struct pvl_span span;
	while((next = pvl_next_span(pvl, next, &span))) {
		if (! span.marked) {
			pvl_detect_leaks_inner(pvl, span.index, span.length);
		}
	}
}

static void pvl_detect_leaks_inner(struct pvl *pvl, size_t from, size_t to) {
	size_t in_diff = 0;
	size_t diff_start = 0;
	for (size_t i = from; i <= to; i++) {
		if (in_diff) {
			if (pvl->main[i] == pvl->mirror[i]) {
				/* Report diff */
				pvl->leak_cb(pvl->leak_ctx, (pvl->main)+diff_start, i-diff_start);
				/* Clear trackers */
				in_diff = 0;
				diff_start = 0;
			}
		} else {
			if (pvl->main[i] != pvl->mirror[i]) {
				/* Set trackers */
				in_diff = 1;
				diff_start = i;
			}
		}
	}
}
