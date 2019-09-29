#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(converters)
add_subdirectory(multibase_codec)

add_library(uvarint
  uvarint.cpp
  )

target_link_libraries(uvarint
  outcome
  hexutil
  )


add_library(multihash
  multihash.cpp
  )

target_link_libraries(multihash
  hexutil
  uvarint
  outcome
  )


add_library(multiaddress
  multiaddress.cpp
  )
target_link_libraries(multiaddress
  converter_utils
  outcome
  )
