MAKEFLAGS += --no-builtin-rules

.SUFFIXES:

CC=clang
AR=ar
CFLAGS=-DWARNING_DO_NOT_INCLUDE_PLV_C -g -fstrict-aliasing -fstack-protector-all -pedantic -Wall -Wextra -Werror -Wfatal-errors --coverage

test: check_coverage check_static

check_coverage:
	rm -f *.gcda
	./tests.out
	llvm-cov-7 gcov -b tests.c | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
	! grep  '#####:' *.gcov
	! grep -E '^branch\s*[0-9]? never executed$$' *.gcov

check_static:
	clang-tidy --checks='*' -warnings-as-errors='*' tests.c pvl.c -- -DWARNING_DO_NOT_INCLUDE_PLV_C
	cppcheck --error-exitcode=1 tests.c pvl.c

tests.out: tests.c libpvl.a
	$(CC) -static $(CFLAGS) tests.c libpvl.a -o tests.out

libpvl.o: pvl.c pvl.h
	$(CC) $(CFLAGS) -c pvl.c -o $@

libpvl.a: libpvl.o
	$(AR) rcs $@ libpvl.o

clean:
	rm -f *.a *.o *.gcda *.gcno *.gcov tests.out

.PHONY: clean test check_coverage check_static
