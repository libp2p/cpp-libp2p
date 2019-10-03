# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(tcp_integration_test
    tcp_integration_test.cpp
    )
target_link_libraries(tcp_integration_test
    p2p_tcp
    p2p_testutil
    p2p_literals
    )


addtest(tcp_listener_test
    tcp_listener_test.cpp
    )
target_link_libraries(tcp_listener_test
    p2p_tcp_listener
    p2p_literals
    )
