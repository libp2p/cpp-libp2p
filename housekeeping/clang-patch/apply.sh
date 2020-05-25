#!/bin/bash
set -e
if ([ "$RUNNER_OS" = "macOS" ] && which clang++ && ! clang++ -fcoalesce-templates --version) >> null; then
  dir=$(cd `dirname $0` && pwd)
  exe="$dir/clang-patch"
  cc -o $exe "$dir/clang-patch.c"
  echo "-DCMAKE_CXX_COMPILER=$exe"
fi
