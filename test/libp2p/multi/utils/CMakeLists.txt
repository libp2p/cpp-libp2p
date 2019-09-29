#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(uvarint_test
    uvarint_test.cpp
    )

target_link_libraries(uvarint_test
    uvarint
    hexutil
    )

addtest(protocol_list_test
    protocol_list_test.cpp
    )

addtest(address_converter_test
    address_converter_test.cpp
    )

target_link_libraries(address_converter_test
    uvarint
    converter_utils
    )

addtest(string_to_bytes_test
    string_from_to_bytes_test.cpp
    )

target_link_libraries(string_to_bytes_test
    converter_utils
    )
