#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(basic)
add_subdirectory(common)
add_subdirectory(connection)
add_subdirectory(crypto)
add_subdirectory(host)
add_subdirectory(log)
add_subdirectory(multi)
add_subdirectory(muxer)
add_subdirectory(network)
add_subdirectory(peer)
add_subdirectory(protocol)
add_subdirectory(protocol_muxer)
add_subdirectory(security)
add_subdirectory(storage)
add_subdirectory(transport)

add_library(p2p
    libp2p.cpp
    )
target_link_libraries(p2p INTERFACE
    p2p_default_host
    p2p_default_network
    )
libp2p_install(p2p)
