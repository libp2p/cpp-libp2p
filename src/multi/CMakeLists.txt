#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(converters)
add_subdirectory(multibase_codec)

add_library(p2p_uvarint
    uvarint.cpp
    )
target_link_libraries(p2p_uvarint
    Boost::boost
    p2p_hexutil
    )


add_library(p2p_multihash
    multihash.cpp
    )
target_link_libraries(p2p_multihash
    p2p_hexutil
    p2p_uvarint
    Boost::boost
    )


add_library(p2p_multiaddress
    multiaddress.cpp
    )
target_link_libraries(p2p_multiaddress
    p2p_converter_utils
    Boost::boost
    )
