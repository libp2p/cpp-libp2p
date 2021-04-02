#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(libp2p_sqlite_test
    sqlite_test.cpp
    )
target_link_libraries(libp2p_sqlite_test
    Boost::filesystem
    p2p_sqlite
    )
