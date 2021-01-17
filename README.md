# Overview
Libpvl is a system prevalence library for C. It uses the addressing capabilities of C to track changes on a block of memory and persist them.

# Licensing

libpvl is licensed under the 0BSD license. See the LICENSE.md file for details.

# Status

[![cicd](https://github.com/spaskalev/libpvl/workflows/cicd/badge.svg)](https://github.com/spaskalev/libpvl/actions)

# Design

## Abstract IO

libpvl is designed to abstract domain code from IO handling. Domain code has access to a few functions (pvl_mark, pvl_commit) that in turn call callbacks to handle the IO. This makes it easy to use different IO handlers without performing any changes on the domain code. The IO handlers can be anything from no-op or in-memory ones for testing, through regular file-based I/O or even over-the-network replication for high availability. Once the libpvl core is complete development will shift towards a set of IO handlers.

## Ease of use

libpvl is designed to be developer-friendly through a small and well-defined API, a defensive programming style and a comprehensive test suite (complete branch and line coverage). It includes troubleshooting facilities to help developers find bugs and tune their applications.

# Usage

## Requirements

libpvl's IO handing is done through callbacks. There are two IO callbacks - a read (from) and a write (to). Both are called with a caller-provided context to pass configuration and with a remaining bytes hints.

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

To achieve constant space complexity for pvl_mark() libpvl splits the main memory block into spans of equal size and uses a bitset to track dirty spans. When pvl_mark() is called it sets the dirty flag for each internal span than overlaps with the caller-provided span. If too few internal spans are used then the space efficiency of the persisted changed can be low. If too many internal spans are used that will increase the size of the pvl object and increase iteration times in pvl_commit().

# Comparison with other prevalence libraries

High-level prevalence libraries like [Prevayler](https://github.com/prevayler/prevayler) for Java and [Madeleine](https://github.com/ghostganz/madeleine) for Ruby wrap changes to the persistent state through serialized command objects. Care is needed to avoid side effects and environment-dependent behavior like "get current timestamp" in commands. Libpvl operates on already-changed raw data and is not affected by this sort of issues. It is also faster by the virtue of doing less - it does not have to serialize/deserialize commands and apply them but just read and write data.

# Caveats

## Pointer care

If pointers are stored by libpvl they will later be restored as-is and might no longer be valid. It is best to avoid pointers in pvl-managed memory. Relative array offsets can be used instead for the same purpose within the managed memory.

## Interoperability

The format that libpvl uses to persist changes is implementation-defined and not part of its contract. Always save and restore using the same libpvl version compiled for the same architecture using the same compiler and settings for maximum compatibility. If you application needs to upgraded and keep its data it might be best to export it in a regular format from the old version and import it back to libpvl-managed memory in the new version.
