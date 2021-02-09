#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

find_package(Threads)

libp2p_add_library(p2p_cares
    cares.cpp
    )
target_link_libraries(p2p_cares
    c-ares::cares
    ${CMAKE_THREAD_LIBS_INIT}
    )
