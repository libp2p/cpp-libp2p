#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(tcp)

addtest(quic_test
    quic_test.cpp
    )
target_link_libraries(quic_test
    p2p_default_host
    p2p_default_network
    )

addtest(libp2p_transport_parser_test
    multiaddress_parser_test.cpp
    )
target_link_libraries(libp2p_transport_parser_test
    p2p_transport_parser
    p2p_literals
    )

addtest(libp2p_upgrader_test
    upgrader_test.cpp
    )
target_link_libraries(libp2p_upgrader_test
    p2p_upgrader
    p2p_multihash
    p2p_multiaddress
    p2p_testutil
    )
