# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(inmem_protocol_repository_test
    inmem_protocol_repository_test.cpp
    )
target_link_libraries(inmem_protocol_repository_test
    p2p_inmem_protocol_repository
    p2p_literals
    )
