/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/reader.hpp>
#include <memory>

namespace libp2p {
  /// Read exactly `out.size()` bytes
  inline void read(const std::shared_ptr<basic::Reader> &reader,
                   BytesOut out,
                   std::function<void(outcome::result<void>)> cb) {
    // read some bytes
    reader->readSome(
        out,
        out.size(),
        [weak{std::weak_ptr{reader}}, out, cb{std::move(cb)}](
            outcome::result<size_t> n_res) mutable {
          if (n_res.has_error()) {
            return cb(n_res.error());
          }
          auto n = n_res.value();
          if (n == out.size()) {
            // successfully read last bytes
            return cb(outcome::success());
          }
          if (n == 0) {
            throw std::logic_error{"libp2p::read zero bytes read"};
          }
          if (n > out.size()) {
            throw std::logic_error{"libp2p::read too much bytes read"};
          }
          // read remaining bytes
          auto reader = weak.lock();
          if (not reader) {
            return cb(std::errc::operation_canceled);
          }
          read(reader, out.subspan(n), std::move(cb));
        });
  }
}  // namespace libp2p
