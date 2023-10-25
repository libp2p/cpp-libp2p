/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "console_async_reader.hpp"

namespace libp2p::protocol::example::utility {

  ConsoleAsyncReader::ConsoleAsyncReader(boost::asio::io_context &io,
                                         Handler handler)
      : in_(io, STDIN_FILENO), handler_(std::move(handler)) {
    read();
  }

  void ConsoleAsyncReader::stop() {
    stopped_ = true;
  }

  void ConsoleAsyncReader::read() {
    input_.consume(input_.data().size());
    boost::asio::async_read_until(
        in_, input_, "\n",
        [this](const boost::system::error_code &e, std::size_t size) {
          onRead(e, size);
        });
  }

  void ConsoleAsyncReader::onRead(const boost::system::error_code &e,
                                  std::size_t size) {
    if (stopped_) {
      return;
    }
    if (!e && size != 0) {
      line_.assign(buffers_begin(input_.data()), buffers_end(input_.data()));
      line_.erase(line_.find_first_of("\r\n"));
      handler_(line_);
    }
    read();
  }

}  // namespace libp2p::protocol::example::utility
