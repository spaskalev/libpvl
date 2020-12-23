# libpvl
libpvl is a system prevalence library for C

# Overview

Compared to other prevalence libraries for high-level languages libpvl is simpler and more bare-boned. It uses the addressing capabilities of C to track changes on a block of memory and persist them.

# Licensing

libpvl is licensed under the 0BSD license. See the LICENSE.md file for details.

# Design

## Abstract IO

libpvl is designed to abstract domain code from IO handling. Domain code has access to a few functions (pvl_mark, pvl_commit) that in turn call callbacks to handle the IO. This makes it easy to use different IO handlers without performing any changes on the domain code. The IO handlers can be anything from no-op or in-memory ones for testing, through regular file-based I/O or even over-the-network replication for high availability. Once the libpvl core is complete development will shift towards a set of IO handlers.

## Ease of use

libpvl is designed to be developer-friendly through a small and well-defined API, a defensive programming style and a comprehensive test suite. It includes troubleshooting facilities to help developers find bugs and tune their applications.

# Usage

## Requirements

libpvl's IO handing is done through callbacks. There are four IO-related callbacks that can be set - pre-save, post-save, pre-load, post-load.

The pre-load callback will be called upon initializing the struct pvl\* object, if set. The pre-load callback should return a FILE\* object from which libpvl will attempt to load a persisted change.

The post-load callback will be called after libpvl has performed a load attempt. It will indicate whether the load  was successful and in turn the post-load callback should indicate whether libpvl should attempt to load another change.

The pre-save callback will be called whenever libpvl deems necessary to persist a change. This can happen for partial changes as well. The pre-save callback should return a FILE\* where libpvl should write its change.

The post-save callback will be called after libpvl has attempted to persist a change. It will indicate whether this was successful.

## Initialization

libpvl's main object type is struct pvl\*, an incomplete type that is initialized by pvl_init(...) at the specified memory location. If the parameters are correct a non-NULL struct pvl\* is returned that can be operated upon by the rest of the functions.

## Making changes

Call pvl_mark(struct pvl\*, char*, size_t) to indicate that a span of memory that has been changed. This operation is cheap to call and idempotent in the context of a commit.

The span location has to fall in the memory block that the struct pvl\* instance has been configured to manage upon calling pvl_init.

## Confirming changes

Call pvl_commit(struct pvl\*) to make libpvl persist the currently-marked spans.

# Troubleshooting

## Detecting leaks

Leaks in libpvl are changes that were not marked when commit was called. This is an issue when restoring state through libpvl as unmarked changes will not be persisted and therefore not restored.

To detect them provide a leak detection callback to pvl_init() and a mirror memory block of the same size as the main memory block. Upon calling pvl_commit()  libpvl will scan the unmarked areas between the main memory block and the mirror and call the leak detection callback for any found leaks. The mirror block is kept up to date with the main block by pvl_commit().

Note that the leak detection will not report leaks that occur in a marked internal span. This is not an immediate problem as these leaks will be persisted and restored. See the section on tuning performance on how libpvl works internally.

## Tuning performance

To achieve constant time and space complexity for pvl_mark() libpvl splits the main memory block into spans of equal size and uses a bitset to track dirty spans. When pvl_mark() is called it sets the dirty flag for each internal span than overlaps with the caller-provided span. If too few internal spans are used then the space efficiency of the persisted changed can be low. If too many internal spans are used that will increase the size of the pvl object and increase iteration times in pvl_commit().

# Comparison with other prevalence libraries

High-level prevalence libraries like [Prevayler](https://github.com/prevayler/prevayler) for Java and [Madeleine](https://github.com/ghostganz/madeleine) for Ruby wrap changes to the persistent state through serialized command objects. Care is needed to avoid side effects and environment-dependent behavior like "get current timestamp" in commands. Libpvl operates on already-changed raw data and is not affected by this sort of issues. It is also faster by the virtue of doing less - it does not have to serialize/deserialize commands and apply them but just read and write data. However libpvl has a similar caveat with pointers - if pointers are stored by it they will later be restored as-is and might no longer be valid. It is best to avoid pointers in pvl-managed memory. Relative array offsets can be used instead for the same purpose within the managed memory.
