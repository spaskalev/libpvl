#
# Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
#

MAKEFLAGS += --no-builtin-rules

.SUFFIXES:

export SHELL = /bin/bash

CC=clang
AR=ar
CFLAGS=-DWARNING_DO_NOT_INCLUDE_PLV_C -g -fstrict-aliasing -fstack-protector-all -pedantic -Wall -Wextra -Werror -Wfatal-errors --coverage
LLVM_COV=$(shell compgen -c | grep llvm-cov | sort | head -n 1)
ALL_SRC=$(wildcard *.c *.h)

test: check_coverage check_static

check_coverage: tests.out
	rm -f *.gcda
	./tests.out
	$(LLVM_COV) gcov -b tests.c | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
	! grep  '#####:' *.gcov
	! grep -E '^branch\s*[0-9]? never executed$$' *.gcov

check_static:
	clang-tidy --checks='*,-llvm-header-guard,-llvm-include-order' -warnings-as-errors='*' $(ALL_SRC) -- -DWARNING_DO_NOT_INCLUDE_PLV_C
	cppcheck --error-exitcode=1 $(ALL_SRC)

tests.out: tests.c libpvl.a bitset.a
	$(CC) -static $(CFLAGS) tests.c libpvl.a bitset.a -o tests.out

libpvl.o: pvl.c pvl.h
	$(CC) $(CFLAGS) -c pvl.c -o $@

libpvl.a: libpvl.o
	$(AR) rcs $@ libpvl.o

bitset.o: bitset.h bitset.c
	$(CC) $(CFLAGS) -c bitset.c -o $@

bitset.a: bitset.o
	$(AR) rcs $@ bitset.o

clean:
	rm -f *.a *.o *.gcda *.gcno *.gcov tests.out

.PHONY: clean test check_coverage check_static
