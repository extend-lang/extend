#!/usr/bin/env sh

# Stage 1: build env

build="cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE=\"-O3 -DNDEBUG -march=native -flto\" -DCMAKE_C_FLAGS_RELEASE=\"-O3 -DNDEBUG -march=native -flto\""
root=/extend

container=$(buildah from alpine:3.13.2)

buildah run $container -- apk add clang lld cmake ninja alpine-sdk linux-headers python3 libunwind-dev libexecinfo-dev flex bison gnu-libiconv grphviz
buildah config \
  --env PATH=$root/tools/env/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  --env CC=/usr/bin/clang \
  --env CXX=/usr/bin/clang++ \
  --env LD=/usr/bin/lld \
  $container

buildah copy $container . $root

buildah run $container sh <<EOF
mkdir -p $root/build/ninja $root/build/cmake $root/build/llvm $root/build/doxygen

cd $root/build/ninja
$build ../../libs/ninja
cmake --build .
cmake --install . --prefix ../../tools/env

cd $root/build/cmake
$build ../../libs/cmake -DCMAKE_USE_OPENSSL=OFF
cmake --build .
cmake --install . --prefix ../../tools/env
EOF

buildah run $container sh <<EOF
cd $root/build/llvm
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -march=native" -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG -march=native" ../../libs/llvm-project/llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;compiler-rt;libcxx;libcxxabi;libunwind;lld;lldb;parallel-libs;pstl" -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_ENABLE_LLD=ON -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_ENABLE_LTO=Thin -DLIBCXX_HAS_MUSL_LIBC=ON -DCOMPILER_RT_SANITIZERS_TO_BUILD=asan,cfi,dfsan,hwsan,msan,tsan
cmake --build .
cmake --install . --prefix ../../tools/env

cd $root/build/doxygen
$build ../../libs/doxygen
cmake --build .
cmake --install . --prefix ../../tools/env
EOF

exit 0

buildah unshare <<EOF
container_dir=\$(buildah mount $container)
rm -rf ./tools/env
cp -r \$container_dir$root/tools/env ./tools
EOF

buildah rm $container

# Stage 2: build extend

container=$(buildah from alpine:3.13.2)

buildah copy $container . $root

buildah run $container -- sh <<EOF
apk add alpine-sdk linux-headers
EOF

buildah config \
  --env PATH=$root/tools/env/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  --env CC=$root/tools/env/bin/clang \
  --env CXX=$root/tools/env/bin/clang++ \
  --env LD=$root/tools/env/bin/lld \
  $container

buildah commit $container extend
