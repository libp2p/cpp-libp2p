# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_ping
    ping.cpp
    ping.hpp
    ping_server_session.cpp
    ping_server_session.hpp
    ping_client_session.cpp
    ping_client_session.hpp
    )
target_link_libraries(libp2p_ping
    Boost::boost
    )
