# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_protocol_echo
    echo.cpp
    server_echo_session.cpp
    client_echo_session.cpp
    )
target_link_libraries(p2p_protocol_echo
    Boost::boost
    p2p_logger
    )
