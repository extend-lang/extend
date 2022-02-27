#!/usr/bin/env sh
set -ex

SCRIPT_PATH=$(which $0)

if [ -L $SCRIPT_PATH ]; then
    SOURCE_DIR=$(dirname $(readlink -f $SCRIPT_PATH))
else
    SOURCE_DIR=$(dirname $SCRIPT_PATH)
fi

export CC=$(which clang)
export CXX=$(which clang++)
export LD=$(which lld)
export AR=$(which llvm-ar)
export NM=$(which llvm-nm)
export RANLIB=$(which llvm-ranlib)
export CFLAGS="-march=native -fPIC -flto=thin"
export CXXFLAGS="-march=native -fPIC -flto=thin -stdlib=libc++ -fno-exceptions -fno-rtti"

CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug}
CMAKE_INSTALL_PREFIX=${PREFIX:-${SOURCE_DIR}/release/$(git describe --tags)}
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}

rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake -GNinja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} ${SOURCE_DIR}

echo "Build inited at '${BUILD_DIR}'"
echo "  run 'cmake --build .' there"
