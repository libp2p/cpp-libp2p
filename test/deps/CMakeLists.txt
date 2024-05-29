#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(di_test di_test.cpp)
target_link_libraries(di_test
    Boost::Boost.DI
    )
