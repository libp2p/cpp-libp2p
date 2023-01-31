/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_ASIO_CB_HPP
#define LIBP2P_COMMON_ASIO_CB_HPP

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

#endif  // LIBP2P_COMMON_ASIO_CB_HPP
