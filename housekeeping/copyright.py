#!/usr/bin/env python3

import os
import itertools
import re

re_cpp = re.compile(r"\.(h|hpp|c|cpp)$")

copyright_cmake = """\
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#
"""
copyright = """\
/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
"""

for dir_, _, names in itertools.chain(*map(os.walk, ["include", "src", "test"])):
    for name in names:
        is_cmake = name == "CMakeLists.txt" or name.endswith(".cmake")
        is_cpp = re_cpp.search(name)
        if not is_cmake and not is_cpp:
            continue
        path = os.path.join(dir_, name)
        txt = open(path).read()
        txt0 = txt
        if not txt.endswith("\n"):
            print("EOL", path)
            txt = txt + "\n"
        if is_cmake and not txt.startswith(copyright_cmake):
            print("COPYRIGHT", path)
            txt = copyright_cmake + "\n" + txt
        if is_cpp and not txt.startswith(copyright):
            print("COPYRIGHT", path)
            txt = copyright + "\n" + txt
        if txt != txt0:
            print("WRITE", path)
            with open(path, "w") as f:
                f.write(txt)
