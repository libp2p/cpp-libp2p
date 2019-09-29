# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(p2p_client_test_session
    client_test_session.hpp
    client_test_session.cpp
    )

target_link_libraries(p2p_client_test_session
    Boost::boost
    )
