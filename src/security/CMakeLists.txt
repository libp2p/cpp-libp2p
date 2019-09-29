#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(plaintext)

add_library(p2p_security_error
    error.cpp
    )
target_link_libraries(p2p_security_error
    outcome
    )
