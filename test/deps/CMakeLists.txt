#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(outcome_test outcome_test.cpp)
target_link_libraries(outcome_test
    Boost::boost
    )

addtest(di_test di_test.cpp)
