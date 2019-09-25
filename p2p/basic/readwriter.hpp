/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READWRITER_HPP
#define KAGOME_READWRITER_HPP

#include "basic/reader.hpp"
#include "basic/writer.hpp"

namespace libp2p::basic {
  struct ReadWriter : public Reader, public Writer {
    ~ReadWriter() override = default;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_READWRITER_HPP
