# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_basic_host
    basic_host.cpp
    )
target_link_libraries(libp2p_basic_host
    outcome
    )
