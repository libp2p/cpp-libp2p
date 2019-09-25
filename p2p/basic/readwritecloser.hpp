/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READWRITECLOSER_HPP
#define KAGOME_READWRITECLOSER_HPP

#include "basic/closeable.hpp"
#include "basic/readwriter.hpp"

namespace libp2p::basic {

  struct ReadWriteCloser : public ReadWriter, public Closeable {
    ~ReadWriteCloser() override = default;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_READWRITECLOSER_HPP
