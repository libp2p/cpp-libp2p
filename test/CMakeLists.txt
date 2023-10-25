#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(acceptance)
add_subdirectory(deps)
add_subdirectory(libp2p)
add_subdirectory(testutil)
