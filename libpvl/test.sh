#!/bin/bash
set -eE
set -x

TEST_SRC="tests.c"
TEST_BIN="tests.out"

LIB_SRC="pvl.c"
LIB_OBJECT="libpvl.o"
LIB_SHARED="libpvl.so"
LIB_STATIC="libpvl.a"

IMPL_GUARD="WARNING_DO_NOT_INCLUDE_PLV_C"

CC="clang"

GC_BASE_OPTS="-g -fstrict-aliasing -fstack-protector-all -pedantic -Wall -Wextra -Werror -Wfatal-errors"
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
    llvm-cov-7 gcov -b tests.c | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
    if grep  '#####:' *.gcov; then
        echo "Non-covered lines found."
        false
    fi
    if grep -E '^branch\s*[0-9]? never executed$' *.gcov; then
        echo "Non-covered branches found."
        false
    fi
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

function test_coverage() {
    GC_OPT_LEVEL="-Og -O0"
    GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD} --coverage"
    GC_TEST_OPTS="${GC_BASE_OPTS} --coverage"
    rm -f *.gcov *.gcno *.gcda
    #test_shared $@
    test_static $@
    verify_coverage $@
}

function test_optlevels() {
    OPTS="-O1 -O2 -O3 -Os"
    for opt in $OPTS; do
        GC_OPT_LEVEL=${opt}
        GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD}"
        GC_TEST_OPTS="${GC_BASE_OPTS}"
        test_shared $@
        test_static $@
    done
}

function sca_checks() {
	cppcheck --error-exitcode=1 $@
}

function test() {
    test_coverage $@
    sca_checks $@
    #test_optlevels $@
}

test ${TEST_SRC}

echo "All tests passed successfully!"
