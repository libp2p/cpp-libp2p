/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/closeable.hpp>
#include <libp2p/basic/readwriter.hpp>

namespace libp2p::basic {

  struct ReadWriteCloser : public ReadWriter, public Closeable {
    ~ReadWriteCloser() override = default;
  };

}  // namespace libp2p::basic
