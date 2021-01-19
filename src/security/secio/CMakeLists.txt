#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

libp2p_add_library(p2p_secio
        secio.cpp
        secio_connection.cpp
        secio_dialer.cpp
        )
target_link_libraries(p2p_secio
        Boost::boost
        p2p_secio_propose_message_marshaller
        p2p_secio_exchange_message_marshaller
        p2p_secio_proto
        p2p_protobuf_message_read_writer
        p2p_logger
        p2p_crypto_error
        p2p_crypto_provider
        p2p_sha
        )

libp2p_add_library(p2p_secio_propose_message_marshaller
        propose_message_marshaller_impl.cpp
        )
target_link_libraries(p2p_secio_propose_message_marshaller
        Boost::boost
        p2p_secio_proto
        p2p_message_read_writer # probably not needed
        )

libp2p_add_library(p2p_secio_exchange_message_marshaller
        exchange_message_marshaller_impl.cpp
        )
target_link_libraries(p2p_secio_exchange_message_marshaller
        Boost::boost
        p2p_secio_proto
        p2p_message_read_writer # probably not needed
        )
