# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(libp2p_inmem_address_repository_test
    inmem_address_repository_test.cpp
    )
target_link_libraries(libp2p_inmem_address_repository_test
    p2p_inmem_address_repository
    p2p_literals
    )
