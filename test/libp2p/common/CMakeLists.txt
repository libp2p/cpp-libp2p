#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

##
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#
##

addtest(byteutil_test
    byteutil_test.cpp
    )
target_link_libraries(byteutil_test
    p2p_byteutil
    )

addtest(metrics_test
    metrics_test.cpp
    )
