#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_converter_utils
    converter_utils.cpp
    )
target_link_libraries(p2p_converter_utils
    p2p_conversion_error
    p2p_uvarint
    p2p_hexutil
    p2p_converters
    )

libp2p_add_library(p2p_conversion_error
    conversion_error.cpp
    )
target_link_libraries(p2p_conversion_error
    Boost::boost
    )

libp2p_add_library(p2p_converters
    ip_v4_converter.cpp
    ip_v6_converter.cpp
    tcp_converter.cpp
    udp_converter.cpp
    ipfs_converter.cpp
    )
target_link_libraries(p2p_converters
    Boost::boost
    p2p_hexutil
    p2p_uvarint
    p2p_multibase_codec
    )
