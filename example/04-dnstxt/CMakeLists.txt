#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(dnstxt
    dnstxt.cpp
    )
target_link_libraries(dnstxt
    c-ares::cares
    )
