/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/reader.hpp>
#include <libp2p/basic/writer.hpp>

namespace libp2p::basic {
  struct ReadWriter : public Reader, public Writer {
    ~ReadWriter() override = default;
  };
}  // namespace libp2p::basic
