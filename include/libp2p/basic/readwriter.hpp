/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_READWRITER_HPP
#define LIBP2P_READWRITER_HPP

#include <libp2p/basic/reader.hpp>
#include <libp2p/basic/writer.hpp>

namespace libp2p::basic {
  struct ReadWriter : public Reader, public Writer {
    ~ReadWriter() override = default;
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_READWRITER_HPP
