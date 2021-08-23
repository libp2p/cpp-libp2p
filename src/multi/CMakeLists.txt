#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(converters)
add_subdirectory(multibase_codec)

libp2p_add_library(p2p_uvarint
    uvarint.cpp
    )
target_link_libraries(p2p_uvarint
    Boost::boost
    p2p_hexutil
    p2p_logger
    )


libp2p_add_library(p2p_multihash
    multihash.cpp
    )
target_link_libraries(p2p_multihash
    p2p_hexutil
    p2p_varint_prefix_reader
    Boost::boost
    )


libp2p_add_library(p2p_multiaddress
    multiaddress.cpp
    )
target_link_libraries(p2p_multiaddress
    p2p_converters
    Boost::boost
    )

libp2p_add_library(p2p_cid
    content_identifier.cpp
    content_identifier_codec.cpp
    )

target_link_libraries(p2p_cid
    p2p_multihash
    p2p_sha
    p2p_uvarint
    p2p_multibase_codec
    )
