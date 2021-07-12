/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/message_read_writer_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::basic, MessageReadWriterError, e) {
  using E = libp2p::basic::MessageReadWriterError;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::BUFFER_IS_EMPTY:
      return "empty buffer provided";
    case E::VARINT_EXPECTED:
      return "varint expected at the beginning of Protobuf message";
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}
