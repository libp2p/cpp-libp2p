[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Git Hooks

This folder _might_ contain some git-hooks to execute various checkup in Kagome.

To activate presented (_and all future_) hooks just execute **once** shell-script [activate_hooks.sh](./activate_hooks.sh) from Kagome project root path. 

## pre-commit

This hook check existing `clang-format` and `cmake-format` and their versions.
If they have recommended version, specific checkup is enabled.

Each changed C++ file (`.hpp` and `.cpp` extensions) gets processed by `clang-format`.

Each changed CMake file (`CMakeLists.txt` and `.cmake` extension) gets processed by `cmake-format`

Commit will be blocked while there are any differences between original files and `clang-format/cmake-format` output files.

## etc.

_Other hooks might be provided by maintainers in the future._

