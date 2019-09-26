#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(yamux_integration_test
    yamux_integration_test.cpp
    )
target_link_libraries(yamux_integration_test
    libp2p_yamux
    multiaddress
    libp2p_plaintext
    libp2p_tcp
    testutil
    )

addtest(yamux_acceptance_test
    yamux_acceptance_test.cpp
    )
target_link_libraries(yamux_acceptance_test
    libp2p_yamux
    libp2p_yamuxed_connection
    multiaddress
    libp2p_plaintext
    libp2p_tcp
    testutil
    )

addtest(yamux_frame_test
    yamux_frame_test.cpp
    )
target_link_libraries(yamux_frame_test
    libp2p_yamuxed_connection
    )
