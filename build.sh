#!/usr/bin/env bash

set -Eeuo pipefail
trap cleanup SIGINT SIGTERM ERR EXIT

SCRIPT_PATH=$(which $0)

if [ -L $SCRIPT_PATH ]; then
    SOURCE_DIR=$(dirname $(readlink -f $SCRIPT_PATH))
else
    SOURCE_DIR=$(dirname $SCRIPT_PATH)
fi


usage() {
  cat <<EOF
Usage: $(basename "${BASH_SOURCE[0]}") [-h] [-v] [--init]

Script description here.

Available options:

-h, --help      Print this help and exit
-v, --verbose   Print script debug info
    --init      It is first build, init repo
    --release   Build the release, instead of debug
EOF
  exit
}

cleanup() {
  trap - SIGINT SIGTERM ERR EXIT
  # script cleanup here
}

setup_colors() {
  if [[ -t 2 ]] && [[ -z "${NO_COLOR-}" ]] && [[ "${TERM-}" != "dumb" ]]; then
    NOFORMAT='\033[0m' RED='\033[0;31m' GREEN='\033[0;32m' ORANGE='\033[0;33m' BLUE='\033[0;34m' PURPLE='\033[0;35m' CYAN='\033[0;36m' YELLOW='\033[1;33m'
  else
    NOFORMAT='' RED='' GREEN='' ORANGE='' BLUE='' PURPLE='' CYAN='' YELLOW=''
  fi
}

msg() {
  echo >&2 -e "${1-}"
}

die() {
  local msg=$1
  local code=${2-1} # default exit status 1
  msg "$msg"
  exit "$code"
}

parse_params() {
  # default values of variables set from params
  init=0
  CMAKE_BUILD_TYPE=Debug

  while :; do
    case "${1-}" in
    -h | --help) usage ;;
    -v | --verbose) set -x ;;
    --no-color) NO_COLOR=1 ;;
    --init) init=1 ;;
    --release) CMAKE_BUILD_TYPE=Release ;;
    -?*) die "Unknown option: $1" ;;
    *) break ;;
    esac
    shift
  done

  args=("$@")

  [[ ${#args[@]} -gt 0 ]] && die "Unexpected script argument '${args}'"

  return 0
}

parse_params "$@"
setup_colors

# script logic here

cd "${SOURCE_DIR}"

if [[ $init -eq 1 ]] ; then
  git submodule deinit -f .
  git submodule update --init
  ./third_party/clang-format-hooks/git-pre-commit-format install || true
  cd "${SOURCE_DIR}/third_party/EASTL"
  git submodule deinit -f .
  git submodule update --init
  cd "${SOURCE_DIR}/third_party/EASTL/test/packages/EAThread"
  git am "${SOURCE_DIR}/third_party/patches/EAThread/"*.patch
else
  if [[ ! -e "${SOURCE_DIR}/compile_commands.json" ]] ; then
    die "Run ${RED}$(basename "${BASH_SOURCE[0]}") --init${NOFORMAT}"
  fi
fi

export CC=$(which clang)
export CXX=$(which clang++)
export LD=$(which lld)
export AR=$(which llvm-ar)
export NM=$(which llvm-nm)
export RANLIB=$(which llvm-ranlib)
export CFLAGS="-flto=thin"
export CXXFLAGS="-flto=thin -stdlib=libc++"
export LDFLAGS="-fuse-ld=lld -flto=thin"

BUILD_DIR=${SOURCE_DIR}/build/extend-${CMAKE_BUILD_TYPE}

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake -GNinja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${SOURCE_DIR}
cmake --build .

if [[ $init -eq 1 ]] ; then
  rm -rf "${SOURCE_DIR}/compile_commands.json"
  ln -s "${BUILD_DIR}/compile_commands.json" "${SOURCE_DIR}/compile_commands.json"
fi

ctest
cpack -C CPackConfig.cmake
