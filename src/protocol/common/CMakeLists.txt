# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_protocol_common
    scheduler.cpp
    )
target_link_libraries(p2p_protocol_common
    Boost::boost
    p2p_logger
    )
