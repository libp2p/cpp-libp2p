# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0



add_library(p2p_transport_parser
    multiaddress_parser.cpp
    )
target_link_libraries(p2p_transport_parser
    Boost::boost
    p2p_multiaddress
    )

add_library(p2p_upgrader
    upgrader_impl.cpp
    )
target_link_libraries(p2p_upgrader
    outcome
    )


add_library(p2p_upgrader_session
    upgrader_session.cpp
    )
target_link_libraries(p2p_upgrader_session
    outcome
    p2p_upgrader
    )
