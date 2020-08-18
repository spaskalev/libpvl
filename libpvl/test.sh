#!/bin/bash
set -e
set -x

TEST_SRC_IFACE="tests_iface.c"
TEST_SRC_IMPL="tests_impl.c"
# Use a single test binary - this way debug.sh stays simple :)
TEST_BIN="tests.out"

LIB_SRC="pvl.c"
LIB_OBJECT="libpvl.o"
LIB_SHARED="libpvl.so"
LIB_STATIC="libpvl.a"

IMPL_GUARD="WARNING_DO_NOT_INCLUDE_PLV_C"

GC_BASE_OPTS="-g -fstrict-aliasing -Wall -Wextra"

GC_LIB_OPTS="${GC_BASE_OPTS} -D${IMPL_GUARD}"
GC_TEST_OPTS="${GC_BASE_OPTS} -fprofile-arcs -ftest-coverage -lgcov --coverage"

# Clear
rm -f "${TEST_BIN} ${LIB_OBJECT} ${LIB_SHARED} ${LIB_STATIC}" *.gcda *.gcno

# Compile and link the lib (shared)
gcc ${GC_LIB_OPTS} -c -fpic ${LIB_SRC} -o ${LIB_OBJECT}
gcc -shared ${LIB_OBJECT} -o ${LIB_SHARED}

# Compile and archive the lib (static)
gcc ${GC_LIB_OPTS} -c ${LIB_SRC} -o ${LIB_OBJECT}
ar rcs ${LIB_STATIC} ${LIB_OBJECT}

# Compile and run the iface tests (shared)
gcc ${GC_TEST_OPTS} -L. -lpvl ${TEST_SRC_IFACE} -o ${TEST_BIN}
LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}" ./${TEST_BIN}

# Compile and run the iface tests (static)
gcc -static ${GC_TEST_OPTS} ${TEST_SRC_IFACE} ${LIB_STATIC} -o ${TEST_BIN}
./${TEST_BIN}

# Compile and run the impl tests (shared)
gcc ${GC_TEST_OPTS} -L. -lpvl ${TEST_SRC_IMPL} -o ${TEST_BIN}
LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}" ./${TEST_BIN}

# Compile and run the impl tests (static)
gcc -static ${GC_TEST_OPTS} ${TEST_SRC_IMPL} ${LIB_STATIC} -o ${TEST_BIN}
./${TEST_BIN}
