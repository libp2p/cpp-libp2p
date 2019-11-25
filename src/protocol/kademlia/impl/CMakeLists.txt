# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

#libp2p_add_library(p2p_protocol_kademlia
##    kad_impl.cpp
#    kad_message.cpp
##    message_read_writer.cpp
##    kad_server_session_impl.cpp
#    )
#target_link_libraries(p2p_protocol_kademlia
#    p2p_peer_id
#    p2p_kad_proto
#    Boost::boost
#    )


libp2p_add_library(p2p_kad
    routing_table_impl.cpp
    scheduler.cpp
    kad_impl.cpp
    kad_message.cpp
    kad_protocol_session.cpp
    )
target_link_libraries(p2p_kad
    Boost::boost
    p2p_peer_id
    p2p_kad_proto
    )
