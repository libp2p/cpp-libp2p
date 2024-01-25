/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/enum_error_code.hpp>

namespace libp2p::basic {
  enum class MessageReadWriterError {
    SUCCESS = 0,
    BUFFER_IS_EMPTY,
    VARINT_EXPECTED,
    INTERNAL_ERROR
  };
}

OUTCOME_HPP_DECLARE_ERROR(libp2p::basic, MessageReadWriterError)
