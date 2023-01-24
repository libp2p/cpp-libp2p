/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_ASIO_STREAM_HPP
#define LIBP2P_CONNECTION_ASIO_STREAM_HPP

#include <boost/asio/io_context.hpp>
#include <memory>

namespace boost::asio {
  class io_context;
}

namespace libp2p::basic {
  struct ReadWriter;
}

namespace libp2p::connection {
  /**
   * Wraps libp2p stream into asio interface
   */
  struct AsioStream {
    using lowest_layer_type = AsioStream;
    lowest_layer_type &lowest_layer();
    const lowest_layer_type &lowest_layer() const;

    using executor_type = boost::asio::io_context::executor_type;
    executor_type get_executor();

    template <typename Handler>
    auto handlerCb(Handler &&handler);

    template <typename MutableBufferSequence, typename Handler>
    void async_read_some(const MutableBufferSequence &buffers,
                         Handler &&handler);

    template <typename ConstBufferSequence, typename Handler>
    void async_write_some(const ConstBufferSequence &buffers,
                          Handler &&handler);

    std::shared_ptr<basic::ReadWriter> conn_;
    boost::asio::io_context &io_context_;
  };
}  // namespace libp2p::connection

#endif  // LIBP2P_CONNECTION_ASIO_STREAM_HPP
