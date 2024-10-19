#!/bin/bash

set -e

TEST_CMD="python3 -m pytest --maxfail=99 test/"

# Make sure binaries can be accessed when invoked by root.
umask 0022

# There are tests that run as root but without CAP_DAC_OVERRIDE. To allow these
# to launch built binaries, the directory tree must be accessible to the root
# user. Since the source directory isn't necessarily accessible to root, we
# build and run tests in a temporary directory that we can set up to be world
# readable/executable.
SOURCE_DIR="$(readlink -f .)"
TEST_DIR="$(mktemp -dt libfuse-build-XXXXXX)"
chmod 0755 "${TEST_DIR}"
cd "${TEST_DIR}"
echo "Running in ${TEST_DIR}"

cp -v "${SOURCE_DIR}/test/lsan_suppress.txt" .
export LSAN_OPTIONS="suppressions=$(pwd)/lsan_suppress.txt"
export ASAN_OPTIONS="detect_leaks=1"
export CC

# Standard build
for CC in gcc gcc-9 gcc-10 clang; do
    echo "=== Building with ${CC} ==="
    mkdir build-${CC}; cd build-${CC}
    if [ "${CC}" == "clang" ]; then
        export CXX="clang++"
        export TEST_WITH_VALGRIND=false
    else
        export TEST_WITH_VALGRIND=true
    fi
    if [ ${CC} == 'gcc-7' ]; then
        build_opts='-D b_lundef=false'
    else
        build_opts=''
    fi
    if [ ${CC} == 'gcc-10' ]; then
        build_opts='-Dc_args=-flto=auto'
    else
        build_opts=''
    fi
    meson -D werror=true ${build_opts} "${SOURCE_DIR}" || (cat meson-logs/meson-log.txt; false)
    ninja

    sudo chown root:root util/fusermount3
    sudo chmod 4755 util/fusermount3
    ${TEST_CMD}
    cd ..
done
(cd build-$CC; sudo ninja install)

sanitized_build()
{
    san=$1
    additonal_option=$2

    echo "=== Building with clang and ${san} sanitizer ==="
    [ -n ${additonal_option} ] || echo "Additional option: ${additonal_option}"

    mkdir build-${san}; pushd build-${san}

    # b_lundef=false is required to work around clang
    # bug, cf. https://groups.google.com/forum/#!topic/mesonbuild/tgEdAXIIdC4
    meson -D b_sanitize=${san} -D b_lundef=false -D werror=true\
           ${additonal_option} "${SOURCE_DIR}" \
           || (cat meson-logs/meson-log.txt; false)
    ninja

    # Test as root and regular user
    sudo ${TEST_CMD}
    sudo chown root:root util/fusermount3
    sudo chmod 4755 util/fusermount3
    # Cleanup temporary files (since they are now owned by root)
    sudo rm -rf test/.pytest_cache/ test/__pycache__

    ${TEST_CMD}
    
    popd
    rm -fr build-${san}
}

# Sanitized build
CC=clang
CXX=clang++
TEST_WITH_VALGRIND=false
for san in undefined address; do
    sanitized_build ${san}
done

# Sanitized build without libc versioned symbols
CC=clang
CXX=clang++
for san in undefined address; do
    sanitized_build ${san} "-Ddisable-libc-symbol-version=true"
done

# Documentation.
(cd "${SOURCE_DIR}"; doxygen doc/Doxyfile)

# Clean up.
rm -rf "${TEST_DIR}"
