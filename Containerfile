FROM fedora:34

RUN dnf -y update \
    && dnf -y groupinstall "Development Tools" "Development Libraries" \
    && dnf -y install clang-devel-12.0.0 llvm-devel-12.0.0 libcxx-devel-12.0.0 compiler-rt-12.0.0 libcxxabi-devel-12.0.0 libcxxabi-static-12.0.0 libcxx-static-12.0.0 lld-12.0.0 libunwind-devel-1.4.0 glibc-static-2.33 cmake ninja-build doxygen \
    && dnf clean all

ENV \
  PROJECT_ROOT=/opt/extend \
  CMAKE_BUILD_TYPE=Release

COPY . ${PROJECT_ROOT}

RUN cd ${PROJECT_ROOT} \
    && ./build_in_host.sh \
    && cd build/extend \
    && cmake --install .
