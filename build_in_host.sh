#!/usr/bin/env sh
set -ex

export CC=$(which clang)
export CXX=$(which clang++)
export LD=$(which lld)
export AR=$(which llvm-ar)
export NM=$(which llvm-nm)
export RANLIB=$(which llvm-ranlib)
export CFLAGS="-march=native -fPIC -flto=thin"
export CXXFLAGS="-march=native -fPIC -flto=thin -stdlib=libc++ -fno-exceptions -fno-rtti"

CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug}
CMAKE_INSTALL_PREFIX=${PREFIX:-$(pwd)/release}

rm -rf build
mkdir -p build/extend
cd build/extend

cmake -GNinja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} ../..
cmake --build .
