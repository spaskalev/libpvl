# libpvl
libpvl is a system prevalence library for C

## Overview

Compared to other prevalence libraries for high-level languages libpvl is simpler and more bare-boned. It uses the addressing capabilities of C to track changes on a block of memory and persist them.

## Licensing

libpvl is licensed under the 0BSD license. See the LICENSE.md file for details.

## Design

### Abstract IO

libpvl is designed to abstract domain code from IO handling. Domain code has access to a few functions (pvl_begin, pvl_mark, pvl_commit, pvl_rollback) that in turn call callbacks to handle the IO. This makes it easy to use different IO handlers without performing any changes on the domain code. The IO handlers can be anything from no-op or in-memory ones for testing, through regular file-based I/O or even over-the-network replication for high availability. Once the libpvl core is complete development will shift towards a set of IO handlers.

### Ease of use

libpvl is designed to be developer-friendly through a small and well-defined API, a defensive programming style and a comprehensive test suite. It includes troubleshooting facilities to help developers find bugs and tune their applications.

## Usage

### Requirements

libpvl's IO handing is done through callbacks. There are four IO-related callbacks that can be set - pre-save, post-save, pre-load, post-load.

The pre-load callback will be called upon initializing the struct pvl\* object, if set. The pre-load callback should return a FILE\* object from which libpvl will attempt to load a persisted change.

The post-load callback will be called after libpvl has performed a load attempt. It will indicate whether the load  was successful and in turn the post-load callback should indicate whether libpvl should attempt to load another change.

The pre-save callback will be called whenever libpvl deems necessary to persist a change. This can happen for partial changes as well. The pre-save callback should return a FILE\* where libpvl should write its change.

The post-save callback will be called after libpvl has attempted to persist a change. It will indicate whether this was successful.

### Initialization

libpvl's main object type is struct pvl\*, an incomplete type that is initialized by pvl_init(...) at the specified memory location. If the parameters are correct a non-NULL struct pvl\* is returned that can be operated upon by the rest of the functions.

### Before making changes

Call pvl_begin(struct pvl\*) once for each transaction that you want libpvl to handle. The other functions will fail unless a transaction has begun. This is a safety feature to detect transaction-related bugs in calling code.

### After making changes

Call pvl_mark(struct pvl\*, char*, size_t) to inform libpvl of a span of memory that has been changed. This has to happen in a transaction started by pvl_begin. You can call pvl_mark multiple times in a single transaction.

The span location has to fall in the memory span that the struct pvl\* instance has been configured to manage upon calling pvl_init.

### Confirming changes

Call pvl_commit(struct pvl\*) to make libpvl persist the currently-marked spans.

### Canceling changes

Call pvl_rollback(struct pvl\*) to make libpvl abandon the currently-marked spans.

## Troubleshooting

### Detecting leaks

### Tuning performance
