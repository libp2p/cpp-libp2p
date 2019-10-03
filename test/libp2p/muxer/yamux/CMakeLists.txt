#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(yamux_integration_test
    yamux_integration_test.cpp
    )
target_link_libraries(yamux_integration_test
    p2p_yamux
    p2p_multiaddress
    p2p_plaintext
    p2p_tcp
    p2p_testutil
    p2p_literals
    )

addtest(yamux_acceptance_test
    yamux_acceptance_test.cpp
    )
target_link_libraries(yamux_acceptance_test
    p2p_yamux
    p2p_yamuxed_connection
    p2p_multiaddress
    p2p_plaintext
    p2p_tcp
    p2p_testutil
    p2p_literals
    )

addtest(yamux_frame_test
    yamux_frame_test.cpp
    )
target_link_libraries(yamux_frame_test
    p2p_yamuxed_connection
    p2p_testutil
    p2p_literals
    )
