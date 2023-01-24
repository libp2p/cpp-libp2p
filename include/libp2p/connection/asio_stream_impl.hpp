/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_ASIO_STREAM_IMPL_HPP
#define LIBP2P_CONNECTION_ASIO_STREAM_IMPL_HPP

#include <libp2p/connection/asio_stream.hpp>

#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <libp2p/basic/readwriter.hpp>

namespace libp2p::connection {
  AsioStream::lowest_layer_type &AsioStream::lowest_layer() {
    return *this;
  }

  const AsioStream::lowest_layer_type &AsioStream::lowest_layer() const {
    return *this;
  }

  AsioStream::executor_type AsioStream::get_executor() {
    return io_context_.get_executor();
  }

  template <typename Handler>
  auto AsioStream::handlerCb(Handler &&handler) {
    return [handler{std::forward<Handler>(handler)}](
               outcome::result<size_t> _n) mutable {
      boost::system::error_code ec{};
      size_t n{};
      if (_n) {
        n = _n.value();
      } else {
        // TODO: std::error_code and boost::system::error_code mapping
        ec = make_error_code(boost::system::errc::operation_not_supported);
      }
      handler(ec, n);
    };
  }

  template <typename MutableBufferSequence, typename Handler>
  void AsioStream::async_read_some(const MutableBufferSequence &buffers,
                                   Handler &&handler) {
    boost::asio::mutable_buffer buffer{
        boost::asio::detail::buffer_sequence_adapter<
            boost::asio::mutable_buffer,
            MutableBufferSequence>::first(buffers)};
    conn_->readSome(gsl::make_span((uint8_t *)buffer.data(), buffer.size()),
                    buffer.size(), handlerCb(std::forward<Handler>(handler)));
  }

  template <typename ConstBufferSequence, typename Handler>
  void AsioStream::async_write_some(const ConstBufferSequence &buffers,
                                    Handler &&handler) {
    boost::asio::const_buffer buffer{
        boost::asio::detail::buffer_sequence_adapter<
            boost::asio::const_buffer, ConstBufferSequence>::first(buffers)};
    conn_->writeSome(
        gsl::make_span((const uint8_t *)buffer.data(), buffer.size()),
        buffer.size(), handlerCb(std::forward<Handler>(handler)));
  }
}  // namespace libp2p::connection

#endif  // LIBP2P_CONNECTION_ASIO_STREAM_IMPL_HPP
