# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_ping
    ping.cpp
    ping_server_session.cpp
    ping_client_session.cpp
    )
target_link_libraries(p2p_ping
    Boost::boost
    )
