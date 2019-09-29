# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(tcp)

addtest(libp2p_transport_parser_test
    multiaddress_parser_test.cpp
    )
target_link_libraries(libp2p_transport_parser_test
    libp2p_transport_parser
    )

addtest(libp2p_upgrader_test
    upgrader_test.cpp
    )
target_link_libraries(libp2p_upgrader_test
    libp2p_upgrader
    multihash
    testutil
    )
