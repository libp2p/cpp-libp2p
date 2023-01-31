/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_ASIO_BUFFER_HPP
#define LIBP2P_COMMON_ASIO_BUFFER_HPP

#include <boost/asio/buffer.hpp>
#include <libp2p/common/span_size.hpp>

namespace libp2p {
  inline boost::asio::const_buffer asioBuffer(
      const gsl::span<const uint8_t> &s) {
    return {s.data(), spanSize(s)};
  }

  inline boost::asio::mutable_buffer asioBuffer(const gsl::span<uint8_t> &s) {
    return {s.data(), spanSize(s)};
  }

  inline gsl::span<const uint8_t> asioBuffer(
      const boost::asio::const_buffer &s) {
    return {(const uint8_t *)s.data(), (ptrdiff_t)s.size()};
  }

  inline gsl::span<uint8_t> asioBuffer(const boost::asio::mutable_buffer &s) {
    return {(uint8_t *)s.data(), (ptrdiff_t)s.size()};
  }
}  // namespace libp2p

#endif  // LIBP2P_COMMON_ASIO_BUFFER_HPP
