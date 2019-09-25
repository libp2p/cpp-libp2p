# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0



add_library(libp2p_transport_parser
    multiaddress_parser.cpp
    )
target_link_libraries(libp2p_transport_parser
    Boost::boost
    multiaddress
    )

add_library(libp2p_upgrader
    upgrader_impl.hpp
    upgrader_impl.cpp
    )
target_link_libraries(libp2p_upgrader
    outcome
    )


add_library(libp2p_upgrader_session
    upgrader_session.cpp
    )
target_link_libraries(libp2p_upgrader_session
    outcome
    libp2p_upgrader
    )
