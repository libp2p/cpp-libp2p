/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/outcome/outcome.hpp>

namespace libp2p {
  inline auto toAsioCbSize(std::function<void(outcome::result<size_t>)> cb) {
    return [cb{std::move(cb)}](boost::system::error_code ec, size_t n) {
      if (ec) {
        cb(ec);
      } else {
        cb(n);
      }
    };
  }
}  // namespace libp2p
