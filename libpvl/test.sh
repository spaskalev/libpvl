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

GC_BASE_OPTS="-g -O0 -fstrict-aliasing -Wall -Wextra"

GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD} --coverage"
GC_TEST_OPTS="${GC_BASE_OPTS} --coverage"

trap 'on_err' ERR

function on_err() {
    echo "Failure detected!"
}

function __clean() {
    rm -f "${TEST_BIN} ${LIB_OBJECT} ${LIB_SHARED} ${LIB_STATIC}" *.gcda *.gcno
}

function __compile_shared_lib() {
    gcc ${GC_LIB_OPTS} -c -fpic ${LIB_SRC} -o ${LIB_OBJECT}
    gcc -shared ${GC_LIB_OPTS} ${LIB_OBJECT} -o ${LIB_SHARED}
}

function __compile_and_run_shared_test() {
    gcc ${GC_TEST_OPTS} -L. -lpvl ${1} -o ${TEST_BIN}
    LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}" ./${TEST_BIN}
}

function __compile_static_lib() {
    gcc ${GC_LIB_OPTS} -c ${LIB_SRC} -o ${LIB_OBJECT}
    ar rcs ${LIB_STATIC} ${LIB_OBJECT}
}

function __compile_and_run_static_test() {
    gcc -static ${GC_TEST_OPTS} ${1} ${LIB_STATIC} -o ${TEST_BIN}
    ./${TEST_BIN}
}

function __coverage() {
    gcovr --fail-under-line ${LINE_COV}
    gcovr --branches --fail-under-branch ${BRANCH_COV}
}

__clean
__compile_shared_lib
__compile_and_run_shared_test ${TEST_SRC_IFACE}
__compile_and_run_shared_test ${TEST_SRC_IMPL}
__coverage

__clean
__compile_static_lib
__compile_and_run_static_test ${TEST_SRC_IFACE}
__compile_and_run_static_test ${TEST_SRC_IMPL}
__coverage
