#!/bin/bash
set -eE
set -x

TEST_SRC_IFACE="tests_iface.c"
TEST_SRC_IMPL="tests_impl.c"
# Use a single test binary - this way debug.sh stays simple :)
TEST_BIN="tests.out"

LIB_SRC="pvl.c"
LIB_OBJECT="libpvl.o"
LIB_SHARED="libpvl.so"
LIB_STATIC="libpvl.a"

LINE_COV="80"
BRANCH_COV="50"

IMPL_GUARD="WARNING_DO_NOT_INCLUDE_PLV_C"

CC="gcc"

GC_BASE_OPTS="-g -fstrict-aliasing -pedantic -Wall -Wextra"
GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD} --coverage"
GC_TEST_OPTS="${GC_BASE_OPTS} --coverage"

trap 'on_err' ERR

function on_err() {
    echo "Failure detected!"
}

function __clean() {
    rm -f *.o *.so *.a *.out *.gcov *.gcda *.gcno
}

function __compile_shared_lib() {
    ${CC} ${GC_OPT_LEVEL} ${GC_LIB_OPTS} -c -fpic ${LIB_SRC} -o ${LIB_OBJECT}
    ${CC} -shared ${GC_OPT_LEVEL} ${GC_LIB_OPTS} ${LIB_OBJECT} -o ${LIB_SHARED}
}

function __compile_and_run_shared_test() {
    ${CC} ${GC_OPT_LEVEL} ${GC_TEST_OPTS} -L. -lpvl ${1} -o ${TEST_BIN}
    LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}" ./${TEST_BIN}
}

function __compile_static_lib() {
    ${CC} ${GC_OPT_LEVEL} ${GC_LIB_OPTS} -c ${LIB_SRC} -o ${LIB_OBJECT}
    ar rcs ${LIB_STATIC} ${LIB_OBJECT}
}

function __compile_and_run_static_test() {
    ${CC} -static ${GC_OPT_LEVEL} ${GC_TEST_OPTS} ${1} ${LIB_STATIC} -o ${TEST_BIN}
    ./${TEST_BIN}
}

function __coverage() {
	if [ "${GC_OPT_LEVEL}" != "-O0" ]; then
		echo "Skipping coverage for optimization level ${GC_OPT_LEVEL}"
		return 0
	fi
	if [ "${CC}" != "gcc" ]; then
		echo "Skipping coverage for compiler ${CC}"
		return 0
	fi
    gcovr --fail-under-line ${LINE_COV}
    gcovr --branches --fail-under-branch ${BRANCH_COV}
}

function __test_shared() {
    __clean
    __compile_shared_lib
    while [ "${1}" != "" ]; do
        __compile_and_run_shared_test ${1}
        shift
    done
    __coverage
}

function __test_static() {
    __clean
    __compile_static_lib
    while [ "${1}" != "" ]; do
        __compile_and_run_static_test ${1}
        shift
    done
    __coverage
}

function __test() {
    __test_shared $@
    __test_static $@
}

OPTS="-Og -O0 -O1 -O2 -O3 -Os"
for opt in $OPTS; do
	GC_OPT_LEVEL=${opt}
	__test ${TEST_SRC_IFACE} ${TEST_SRC_IMPL}
done
