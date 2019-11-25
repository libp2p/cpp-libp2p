# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_kad2
    kad2.cpp
    kad_message.cpp
    kad_protocol_session.cpp
    low_res_timer.cpp
    )

target_link_libraries(p2p_kad2
    p2p_kad_routing_table
    p2p_kad_proto
    )



