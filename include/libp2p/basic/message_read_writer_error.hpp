/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MESSAGE_READ_WRITER_ERROR_HPP
#define LIBP2P_MESSAGE_READ_WRITER_ERROR_HPP

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::basic {
  enum class MessageReadWriterError {
    SUCCESS = 0,
    BUFFER_IS_EMPTY,
    VARINT_EXPECTED,
    INTERNAL_ERROR
  };
}

OUTCOME_HPP_DECLARE_ERROR(libp2p::basic, MessageReadWriterError)

#endif  // LIBP2P_MESSAGE_READ_WRITER_ERROR_HPP
