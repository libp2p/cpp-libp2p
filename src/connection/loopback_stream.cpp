/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/connection/loopback_stream.hpp>

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
                                        VoidResultHandlerFunc cb) {};

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

  void LoopbackStream::writeSome(BytesIn in,
                                 libp2p::basic::Writer::WriteCallbackFunc cb) {
    if (is_reset_) {
      return deferWriteCallback(Error::STREAM_RESET_BY_HOST, std::move(cb));
    }
    if (!is_writable_) {
      return deferWriteCallback(Error::STREAM_NOT_WRITABLE, std::move(cb));
    }
    if (in.empty()) {
      return deferWriteCallback(Error::STREAM_INVALID_ARGUMENT, std::move(cb));
    }

    if (boost::asio::buffer_copy(
            buffer_.prepare(in.size()),
            boost::asio::const_buffer(in.data(), in.size()))
        != in.size()) {
      return deferWriteCallback(Error::STREAM_INTERNAL_ERROR, std::move(cb));
    }

    buffer_.commit(in.size());

    // intentionally used deferReadCallback, since it acquires bytes written
    deferReadCallback(in.size(), std::move(cb));
    /* The whole approach with such methods (deferReadCallback and
     * deferWriteCallback) is going to be avoided in the near future, thus we do
     * not remove  from the source code the counting of bytes written */

    if (auto data_notifyee = std::move(data_notifyee_)) {
      data_notifyee(buffer_.size());
    }
  }

  void LoopbackStream::readSome(BytesOut out,
                                libp2p::basic::Reader::ReadCallbackFunc cb) {
    if (is_reset_) {
      return deferReadCallback(Error::STREAM_RESET_BY_HOST, std::move(cb));
    }
    if (!is_readable_) {
      return deferReadCallback(Error::STREAM_NOT_READABLE, std::move(cb));
    }
    if (out.empty()) {
      return cb(Error::STREAM_INVALID_ARGUMENT);
    }

    // this lambda checks, if there's enough data in our read buffer, and gives
    // it to the caller, if so
    auto read_lambda = [self{shared_from_this()}, cb{std::move(cb)}, out](
                           outcome::result<size_t> res) mutable {
      if (!res) {
        self->data_notified_ = true;
        self->deferReadCallback(res.as_failure(), std::move(cb));
        return;
      }

      if (self->buffer_.size() > 0) {
        auto to_read = std::min(self->buffer_.size(), out.size());

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
