#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(router_test
    router_test.cpp
    )
target_link_libraries(router_test
    libp2p_router
    multiaddress
    buffer
    libp2p_peer_id
    )


addtest(transport_manager_test
    transport_manager_test.cpp
    )
target_link_libraries(transport_manager_test
    libp2p_transport_manager
    multiaddress
    )


addtest(connection_manager_test
    connection_manager_test.cpp
    )
target_link_libraries(connection_manager_test
    libp2p_connection_manager
    multiaddress
    address_repository
    libp2p_peer_id
    testutil
    libp2p_peer_errors
    )


addtest(listener_manager_test
    listener_manager_test.cpp
    )
target_link_libraries(listener_manager_test
    libp2p_listener_manager
    )


addtest(dialer_test
    dialer_test.cpp
    )
target_link_libraries(dialer_test
    libp2p_dialer
    )
