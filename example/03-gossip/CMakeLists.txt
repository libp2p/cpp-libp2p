#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(gossip_example
    gossip_example.cpp
    )

libp2p_add_library(gossip_example_common
    factory.cpp
    utility.cpp
    )

target_link_libraries(gossip_example_common
    p2p_basic_host
    p2p_default_network
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    p2p_gossip
    asio_scheduler
    )

target_link_libraries(gossip_example
    gossip_example_common
    )

add_executable(gossip_chat_example
    gossip_chat_example.cpp
    )

target_link_libraries(gossip_chat_example
    gossip_example_common
    )
