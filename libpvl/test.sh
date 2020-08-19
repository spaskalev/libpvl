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
}

function __test_static() {
    __clean
    __compile_static_lib
    while [ "${1}" != "" ]; do
        __compile_and_run_static_test ${1}
        shift
    done
}

function __test() {
    OPTS="-Og -O0 -O1 -O2 -O3 -Os"
    for opt in $OPTS; do
        GC_OPT_LEVEL=${opt}
        GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD}"
        GC_TEST_OPTS="${GC_BASE_OPTS}"
        if [ "-O0" == "${opt}" ]; then
            GC_LIB_OPTS="${GC_LIB_OPTS} --coverage"
            GC_TEST_OPTS="${GC_TEST_OPTS} --coverage"
        fi
        __test_shared $@
        if [ "-O0" == "${opt}" ]; then
            __coverage
        fi
        __test_static $@
        if [ "-O0" == "${opt}" ]; then
            __coverage
        fi
    done
}

__test ${TEST_SRC_IFACE} ${TEST_SRC_IMPL}
