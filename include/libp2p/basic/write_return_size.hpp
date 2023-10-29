/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/write.hpp>

namespace libp2p {
  /// Write exactly `in.size()` bytes
  inline void writeReturnSize(const std::shared_ptr<basic::Writer> &writer,
                              BytesIn in,
                              basic::Writer::WriteCallbackFunc cb) {
    write(
        writer, in, [n{in.size()}, cb{std::move(cb)}](outcome::result<void> r) {
          if (r.has_error()) {
            cb(r.error());
          } else {
            cb(n);
          }
        });
  }
}  // namespace libp2p
