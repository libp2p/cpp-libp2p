#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(asio_scheduler_test
    asio_scheduler_test.cpp
    )
target_link_libraries(asio_scheduler_test
    asio_scheduler

    p2p_basic_host
    p2p_default_network
    p2p_peer_repository
    p2p_inmem_address_repository
    p2p_inmem_key_repository
    p2p_inmem_protocol_repository
    p2p_literals
    asio_scheduler
    )
