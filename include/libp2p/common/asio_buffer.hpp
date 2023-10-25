/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_ASIO_BUFFER_HPP
#define LIBP2P_COMMON_ASIO_BUFFER_HPP

#include <boost/asio/buffer.hpp>

#include <libp2p/common/types.hpp>

namespace libp2p {
  inline boost::asio::const_buffer asioBuffer(BytesIn s) {
    return {s.data(), s.size()};
  }

  inline boost::asio::mutable_buffer asioBuffer(BytesOut s) {
    return {s.data(), s.size()};
  }

  inline BytesIn asioBuffer(const boost::asio::const_buffer &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }

  inline BytesOut asioBuffer(const boost::asio::mutable_buffer &s) {
    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<uint8_t *>(s.data()), s.size()};
  }
}  // namespace libp2p

#endif  // LIBP2P_COMMON_ASIO_BUFFER_HPP
