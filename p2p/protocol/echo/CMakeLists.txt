# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_protocol_echo
    echo.cpp
    echo.hpp
    echo_config.hpp
    server_echo_session.cpp
    server_echo_session.hpp
    client_echo_session.cpp
    client_echo_session.hpp
    )
target_link_libraries(libp2p_protocol_echo
    Boost::boost
    logger
    )
