#!/bin/bash -xe

## on master branch: analyzes all cpp files
## on other branches: analyzes cpp files changed in this branch, in comparison to master

function get_abs_path(){
    echo $(echo "$(cd "$(dirname "$1")"; pwd -P)/$(basename "$1")")
}

function get_files(){
    local head=$(git rev-parse --abbrev-ref HEAD)
    if [[ "${head}" = "master" ]]; then
        echo $(find . -type f | grep "cpp")
    else
        echo $(git diff --name-only HEAD..origin/master | grep "cpp" | grep -v "test")
    fi
}

BUILD_DIR=$(get_abs_path $1)
cd $(dirname $0)/..

# list of cpp files changed in this branch (in comparison to master); tests are ignored
FILES=$(get_files)

CLANG_TIDY=$(which clang-tidy)
RUN_CLANG_TIDY=$(which run-clang-tidy.py)

# filter compile_commands.json
echo ${FILES} | python3 ./housekeeping/filter_compile_commands.py -p ${BUILD_DIR}

# exec run-clang-tidy.py
python3 ${RUN_CLANG_TIDY} -clang-tidy-binary=${CLANG_TIDY} -p ${BUILD_DIR} -header-filter "core/.*\.hpp"
