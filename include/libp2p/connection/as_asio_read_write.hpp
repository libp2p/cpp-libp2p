/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_AS_ASIO_READ_WRITE_HPP
#define LIBP2P_CONNECTION_AS_ASIO_READ_WRITE_HPP

#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <boost/asio/io_context.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/common/shared_fn.hpp>
#include <libp2p/connection/layer_connection.hpp>

namespace libp2p {
  struct AsAsioReadWrite {
    struct ErrorCategory : boost::system::error_category {
      const char *name() const BOOST_NOEXCEPT override {
        return "libp2p::AsAsioReadWrite::ErrorCategory";
      }
      std::string message(int) const override {
        return "libp2p::AsAsioReadWrite::error";
      }

      static const ErrorCategory &get() {
        static ErrorCategory self;
        return self;
      }
    };

    static boost::system::error_code error() {
      return {1, ErrorCategory::get()};
    }

    AsAsioReadWrite(std::shared_ptr<boost::asio::io_context> io,
                    std::shared_ptr<connection::LayerConnection> impl)
        : io{std::move(io)}, impl{std::move(impl)} {}

    using lowest_layer_type = AsAsioReadWrite;
    lowest_layer_type &lowest_layer() {
      return *this;
    }
    const lowest_layer_type &lowest_layer() const {
      return *this;
    }

    using executor_type = boost::asio::io_context::executor_type;
    executor_type get_executor() {
      return io->get_executor();
    }

    template <typename Cb>
    auto wrapCb(Cb &&cb) {
      return SharedFn{
          [cb{std::forward<Cb>(cb)}](outcome::result<size_t> _r) mutable {
            if (_r) {
              cb(boost::system::error_code{}, _r.value());
            } else {
              cb(error(), 0);
            }
          }};
    }

    template <typename MutableBufferSequence, typename Cb>
    void async_read_some(const MutableBufferSequence &buffers, Cb &&cb) {
      boost::asio::mutable_buffer buffer{
          boost::asio::detail::buffer_sequence_adapter<
              boost::asio::mutable_buffer,
              MutableBufferSequence>::first(buffers)};
      impl->readSome(asioBuffer(buffer), buffer.size(),
                     wrapCb(std::forward<Cb>(cb)));
    }

    template <typename ConstBufferSequence, typename Cb>
    void async_write_some(const ConstBufferSequence &buffers, Cb &&cb) {
      boost::asio::const_buffer buffer{
          boost::asio::detail::buffer_sequence_adapter<
              boost::asio::const_buffer, ConstBufferSequence>::first(buffers)};
      impl->writeSome(asioBuffer(buffer), buffer.size(),
                      wrapCb(std::forward<Cb>(cb)));
    }

    std::shared_ptr<boost::asio::io_context> io;
    std::shared_ptr<connection::LayerConnection> impl;
  };
}  // namespace libp2p

#endif  // LIBP2P_CONNECTION_AS_ASIO_READ_WRITE_HPP
