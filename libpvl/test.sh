#!/bin/bash
set -eE
set -x

TEST_SRC_IFACE="tests_iface.c"
TEST_SRC_IMPL="tests_impl.c"
TEST_SRC_E2E="tests_e2e.c"
# Use a single test binary - this way debug.sh stays simple :)
TEST_BIN="tests.out"

LIB_SRC="pvl.c"
LIB_OBJECT="libpvl.o"
LIB_SHARED="libpvl.so"
LIB_STATIC="libpvl.a"

LIB_LINE_COV="94"
LIB_BRANCH_COV="93"

TEST_LINE_COV="100"

IMPL_GUARD="WARNING_DO_NOT_INCLUDE_PLV_C"

CC="gcc"

GC_BASE_OPTS="-g -fstrict-aliasing -fstack-protector-all -pedantic -Wall -Wextra -Werror"
GC_SANS_OPTS="-fno-omit-frame-pointer -fsanitize=leak,address,undefined -fno-sanitize-recover=all"

trap 'on_err' ERR

function on_err() {
    echo "Failure detected!"
}

function clean() {
    rm -f *.o *.so *.a *.out *.gcov *.gcda *.gcno
}

function compile_shared_lib() {
    ${CC} ${GC_OPT_LEVEL} ${GC_LIB_OPTS} -c -fpic ${LIB_SRC} -o ${LIB_OBJECT}
    ${CC} -shared ${GC_OPT_LEVEL} ${GC_LIB_OPTS} ${LIB_OBJECT} -o ${LIB_SHARED}
}

function compile_and_run_shared_test() {
    ${CC} ${GC_OPT_LEVEL} ${GC_TEST_OPTS} -L. -lpvl ${1} -o ${TEST_BIN}
    LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}" ./${TEST_BIN}
}

function compile_static_lib() {
    ${CC} ${GC_OPT_LEVEL} ${GC_LIB_OPTS} -c ${LIB_SRC} -o ${LIB_OBJECT}
    ar rcs ${LIB_STATIC} ${LIB_OBJECT}
}

function compile_and_run_static_test() {
    ${CC} -static ${GC_OPT_LEVEL} ${GC_TEST_OPTS} ${1} ${LIB_STATIC} -o ${TEST_BIN}
    ./${TEST_BIN}
}

function verify_coverage() {
    rm gcovr/*.html
    # Generate full coverage report
    gcovr --html-details --output gcovr/coverage.html
    # Enforce lib coverage
    gcovr --exclude 'tests_.*' --fail-under-line ${LIB_LINE_COV}
    gcovr --exclude 'tests_.*' --branches --fail-under-branch ${LIB_BRANCH_COV}
    # Enforce test coverage
    gcovr --filter 'tests_.*' --fail-under-line ${TEST_LINE_COV}
}

function test_shared() {
    clean
    compile_shared_lib
    while [ "${1}" != "" ]; do
        compile_and_run_shared_test ${1}
        shift
    done
}

function test_static() {
    clean
    compile_static_lib
    while [ "${1}" != "" ]; do
        compile_and_run_static_test ${1}
        shift
    done
}

function test_debug() {
    GC_OPT_LEVEL="-Og -O0"
    GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD} ${GC_SANS_OPTS}"
    GC_TEST_OPTS="${GC_BASE_OPTS} ${GC_SANS_OPTS}"
    test_shared $@
    GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD}"
    GC_TEST_OPTS="${GC_BASE_OPTS}"
    test_static $@
}

function test_coverage() {
    if [ "${CC}" != "gcc" ]; then
        echo "Skipping coverage for compiler ${CC}"
        return 0
    fi
    GC_OPT_LEVEL="-O0"
    GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD} --coverage"
    GC_TEST_OPTS="${GC_BASE_OPTS} --coverage"
    test_shared $@
    verify_coverage
    #test_static $@
    #verify_coverage
}

function test_optlevels() {
    OPTS="-O1 -O2 -O3 -Os"
    for opt in $OPTS; do
        GC_OPT_LEVEL=${opt}
        GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD}"
        GC_TEST_OPTS="${GC_BASE_OPTS}"
        test_shared $@
        #test_static $@
    done
}

function test() {
    test_debug $@
    test_coverage $@
    test_optlevels $@
}

test ${TEST_SRC_IFACE} ${TEST_SRC_IMPL} ${TEST_SRC_E2E}
