/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/read.hpp>

namespace libp2p {
  /// Read exactly `out.size()` bytes
  inline void readReturnSize(const std::shared_ptr<basic::Reader> &reader,
                             BytesOut out,
                             basic::Reader::ReadCallbackFunc cb) {
    read(reader,
         out,
         [n{out.size()}, cb{std::move(cb)}](outcome::result<void> r) {
           if (r.has_error()) {
             cb(r.error());
           } else {
             cb(n);
           }
         });
  }
}  // namespace libp2p
