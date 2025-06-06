/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/connection/loopback_stream.hpp>

#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/common/ambigous_size.hpp>

namespace libp2p::connection {

  namespace {
    template <typename Callback>
    void deferCallback(boost::asio::io_context &ctx,
                       std::weak_ptr<LoopbackStream> wptr,
                       Callback cb,
                       outcome::result<size_t> arg) {
      // defers callback to the next event loop cycle,
      // cb will be called iff Loopbackstream is still exists
      boost::asio::post(
          ctx,
          [wptr{std::move(wptr)}, cb{std::move(cb)}, arg{std::move(arg)}]() {
            if (not wptr.expired()) {
              cb(arg);
            }
          });
    }
  }  // namespace

  LoopbackStream::LoopbackStream(
      libp2p::peer::PeerInfo own_peer_info,
      std::shared_ptr<boost::asio::io_context> io_context)
      : own_peer_info_(std::move(own_peer_info)),
        io_context_{std::move(io_context)} {}

  bool LoopbackStream::isClosedForRead() const {
    return !is_readable_;
  };

  bool LoopbackStream::isClosedForWrite() const {
    return !is_writable_;
  };

  bool LoopbackStream::isClosed() const {
    return isClosedForRead() && isClosedForWrite();
  };

  void LoopbackStream::close(VoidResultHandlerFunc cb) {
    is_writable_ = false;
    cb(outcome::success());
  };

  void LoopbackStream::reset() {
    is_reset_ = true;
  };

  void LoopbackStream::adjustWindowSize(uint32_t new_size,
                                        VoidResultHandlerFunc cb){};

  outcome::result<bool> LoopbackStream::isInitiator() const {
    return outcome::success(false);
  };

  outcome::result<libp2p::peer::PeerId> LoopbackStream::remotePeerId() const {
    return outcome::success(own_peer_info_.id);
  };

  outcome::result<libp2p::multi::Multiaddress> LoopbackStream::localMultiaddr()
      const {
    return outcome::success(own_peer_info_.addresses.front());
  };

  outcome::result<libp2p::multi::Multiaddress> LoopbackStream::remoteMultiaddr()
      const {
    return outcome::success(own_peer_info_.addresses.front());
  }

  void LoopbackStream::read(BytesOut out,
                            size_t bytes,
                            libp2p::basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    readReturnSize(shared_from_this(), out, std::move(cb));
  }

  boost::asio::awaitable<outcome::result<size_t>> LoopbackStream::read(BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);
    if (is_reset_) {
      co_return Error::STREAM_RESET_BY_HOST;
    }
    if (!is_readable_) {
      co_return Error::STREAM_NOT_READABLE;
    }
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      co_return Error::STREAM_INVALID_ARGUMENT;
    }
    // Wait until enough data is available
    while (buffer_.size() < bytes) {
      co_await boost::asio::post(*io_context_, boost::asio::use_awaitable);
    }
    auto to_read = std::min(buffer_.size(), bytes);
    if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), to_read), buffer_.data(), to_read) != to_read) {
      co_return Error::STREAM_INTERNAL_ERROR;
    }
    buffer_.consume(to_read);
    co_return to_read;
  }

  void LoopbackStream::writeSome(BytesIn in,
                                 size_t bytes,
                                 libp2p::basic::Writer::WriteCallbackFunc cb) {
    if (is_reset_) {
      return deferWriteCallback(Error::STREAM_RESET_BY_HOST, std::move(cb));
    }
    if (!is_writable_) {
      return deferWriteCallback(Error::STREAM_NOT_WRITABLE, std::move(cb));
    }
    if (bytes == 0 || in.empty() || static_cast<size_t>(in.size()) < bytes) {
      return deferWriteCallback(Error::STREAM_INVALID_ARGUMENT, std::move(cb));
    }

    if (boost::asio::buffer_copy(buffer_.prepare(bytes),
                                 boost::asio::const_buffer(in.data(), bytes))
        != bytes) {
      return deferWriteCallback(Error::STREAM_INTERNAL_ERROR, std::move(cb));
    }

    buffer_.commit(bytes);

    // intentionally used deferReadCallback, since it acquires bytes written
    deferReadCallback(bytes, std::move(cb));
    /* The whole approach with such methods (deferReadCallback and
     * deferWriteCallback) is going to be avoided in the near future, thus we do
     * not remove  from the source code the counting of bytes written */

    if (auto data_notifyee = std::move(data_notifyee_)) {
      data_notifyee(buffer_.size());
    }
  }

  boost::asio::awaitable<std::error_code> LoopbackStream::writeSome(BytesIn in, size_t bytes) {
    if (is_reset_) {
      co_return Error::STREAM_RESET_BY_HOST;
    }
    if (!is_writable_) {
      co_return Error::STREAM_NOT_WRITABLE;
    }
    if (bytes == 0 || in.empty() || static_cast<size_t>(in.size()) < bytes) {
      co_return Error::STREAM_INVALID_ARGUMENT;
    }
    if (boost::asio::buffer_copy(buffer_.prepare(bytes), boost::asio::const_buffer(in.data(), bytes)) != bytes) {
      co_return Error::STREAM_INTERNAL_ERROR;
    }
    buffer_.commit(bytes);
    co_return std::error_code{};
  }

  void LoopbackStream::readSome(BytesOut out,
                                size_t bytes,
                                libp2p::basic::Reader::ReadCallbackFunc cb) {
    if (is_reset_) {
      return deferReadCallback(Error::STREAM_RESET_BY_HOST, std::move(cb));
    }
    if (!is_readable_) {
      return deferReadCallback(Error::STREAM_NOT_READABLE, std::move(cb));
    }
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      return cb(Error::STREAM_INVALID_ARGUMENT);
    }

    // this lambda checks, if there's enough data in our read buffer, and gives
    // it to the caller, if so
    auto read_lambda = [self{shared_from_this()},
                        cb{std::move(cb)},
                        out,
                        bytes](outcome::result<size_t> res) mutable {
      if (!res) {
        self->data_notified_ = true;
        self->deferReadCallback(res.as_failure(), std::move(cb));
        return;
      }

      if (self->buffer_.size() > 0) {
        auto to_read = std::min(self->buffer_.size(), bytes);

        if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), to_read),
                                     self->buffer_.data(),
                                     to_read)
            != to_read) {
          return self->deferReadCallback(Error::STREAM_INTERNAL_ERROR,
                                         std::move(cb));
        }

        self->buffer_.consume(to_read);

        self->data_notified_ = true;
        self->deferReadCallback(to_read, std::move(cb));
      }
    };

    // return immediately, if there's enough data in the buffer
    data_notified_ = false;
    read_lambda(0);
    if (data_notified_) {
      return;
    }

    // subscribe to new data updates
    if (not data_notifyee_) {
      data_notifyee_ = std::move(read_lambda);
    }
  }

  boost::asio::awaitable<outcome::result<size_t>> LoopbackStream::readSome(BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);
    if (is_reset_) {
      co_return Error::STREAM_RESET_BY_HOST;
    }
    if (!is_readable_) {
      co_return Error::STREAM_NOT_READABLE;
    }
    if (bytes == 0 || out.empty() || static_cast<size_t>(out.size()) < bytes) {
      co_return Error::STREAM_INVALID_ARGUMENT;
    }
    // Wait until any data is available
    while (buffer_.size() == 0) {
      co_await boost::asio::post(*io_context_, boost::asio::use_awaitable);
    }
    auto to_read = std::min(buffer_.size(), bytes);
    if (boost::asio::buffer_copy(boost::asio::buffer(out.data(), to_read), buffer_.data(), to_read) != to_read) {
      co_return Error::STREAM_INTERNAL_ERROR;
    }
    buffer_.consume(to_read);
    co_return to_read;
  }

  void LoopbackStream::deferReadCallback(outcome::result<size_t> res,
                                         basic::Reader::ReadCallbackFunc cb) {
    deferCallback(
        *io_context_, weak_from_this(), std::move(cb), std::move(res));
  }

  void LoopbackStream::deferWriteCallback(std::error_code ec,
                                          basic::Writer::WriteCallbackFunc cb) {
    deferCallback(*io_context_, weak_from_this(), std::move(cb), ec);
  };

}  // namespace libp2p::connection
