#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_hexutil
    hexutil.cpp
    )
target_link_libraries(p2p_hexutil
    Boost::boost
    )

libp2p_add_library(p2p_byteutil
    byteutil.cpp
    )
target_link_libraries(p2p_byteutil
    Boost::boost
    )

libp2p_add_library(p2p_literals
    literals.cpp
    )
target_link_libraries(p2p_literals
    p2p_peer_id
    p2p_multihash
    p2p_multiaddress
    p2p_hexutil
    )
