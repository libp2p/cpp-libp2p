/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_READ_HPP
#define LIBP2P_BASIC_READ_HPP

#include <libp2p/basic/reader.hpp>
#include <memory>

namespace libp2p {
  /// Read exactly `out.size()` bytes
  inline void read(const std::shared_ptr<basic::Reader> &reader, BytesOut out,
                   std::function<void(outcome::result<void>)> cb) {
    auto post_cb = [](decltype(reader) reader, decltype(cb) &&cb,
                      outcome::result<size_t> r) {
      reader->deferReadCallback(r,
                                [cb{std::move(cb)}](outcome::result<size_t> r) {
                                  if (r.has_error()) {
                                    cb(r.error());
                                  } else {
                                    cb(outcome::success());
                                  }
                                });
    };
    if (out.empty()) {
      return post_cb(reader, std::move(cb), outcome::success());
    }
    // read some bytes
    reader->readSome(
        out, out.size(),
        [weak{std::weak_ptr{reader}}, out, cb{std::move(cb)},
         post_cb](outcome::result<size_t> n_res) mutable {
          auto reader = weak.lock();
          if (not reader) {
            return;
          }
          if (n_res.has_error()) {
            return post_cb(reader, std::move(cb), n_res.error());
          }
          auto n = n_res.value();
          if (n == 0) {
            throw std::logic_error{"libp2p::read zero bytes read"};
          }
          if (n > out.size()) {
            throw std::logic_error{"libp2p::read too much bytes read"};
          }
          if (n == out.size()) {
            // successfully read last bytes
            return post_cb(reader, std::move(cb), outcome::success());
          }
          // read remaining bytes
          read(reader, out.subspan(n), std::move(cb));
        });
  }
}  // namespace libp2p

#endif  // LIBP2P_BASIC_READ_HPP
