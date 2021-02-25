# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(p2p_scheduler
    scheduler.cpp
    )
target_link_libraries(p2p_scheduler
    Boost::boost
    p2p_logger
    )

libp2p_add_library(asio_scheduler
    asio/asio_scheduler.cpp
    )
target_link_libraries(asio_scheduler
    p2p_scheduler
    )

libp2p_add_library(subscription
    subscription.cpp
    )
