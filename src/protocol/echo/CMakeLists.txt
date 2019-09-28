# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_protocol_echo
    echo.cpp
    server_echo_session.cpp
    client_echo_session.cpp
    )
target_link_libraries(libp2p_protocol_echo
    Boost::boost
    logger
    )
