# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(outcome INTERFACE IMPORTED GLOBAL)
set_target_properties(outcome PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}
  INTERFACE_LINK_LIBRARIES Boost::boost
  )
