/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_READ_RETURN_SIZE_HPP
#define LIBP2P_BASIC_READ_RETURN_SIZE_HPP

#include <libp2p/basic/read.hpp>

namespace libp2p {
  /// Read exactly `out.size()` bytes
  inline void readReturnSize(const std::shared_ptr<basic::Reader> &reader,
                             MutSpanOfBytes out,
                             basic::Reader::ReadCallbackFunc cb) {
    read(reader, out,
         [n{out.size()}, cb{std::move(cb)}](outcome::result<void> r) {
           if (r.has_error()) {
             cb(r.error());
           } else {
             cb(n);
           }
         });
  }
}  // namespace libp2p

#endif  // LIBP2P_BASIC_READ_RETURN_SIZE_HPP
