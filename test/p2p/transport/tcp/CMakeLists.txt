# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(tcp_integration_test
    tcp_integration_test.cpp
    )
target_link_libraries(tcp_integration_test
    libp2p_tcp
    testutil
    )


addtest(tcp_listener_test
    tcp_listener_test.cpp
    )
target_link_libraries(tcp_listener_test
    libp2p_tcp_listener
    )
