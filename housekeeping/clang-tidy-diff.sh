#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

BUILD_DIR="${BUILD_DIR:?}"

cd $(dirname $0)/..
git diff -U0 origin/master | clang-tidy-diff.py -p1 -path $BUILD_DIR -iregex '(include|src)\/.*\.(h|c|hpp|cpp)' -clang-tidy-binary clang-tidy-15 | tee clang-tidy.log
! grep ': error:' clang-tidy.log
