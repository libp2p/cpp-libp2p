/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/connection/stream_pair.hpp>

#include <boost/asio/streambuf.hpp>
#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/ambigous_size.hpp>
#include <libp2p/connection/stream.hpp>
#include <qtils/option_take.hpp>

namespace libp2p::connection {
  class StreamPair : public std::enable_shared_from_this<StreamPair>,
                     public Stream {
    friend std::pair<std::shared_ptr<Stream>, std::shared_ptr<Stream>>
    streamPair(std::shared_ptr<basic::Scheduler> post,
               PeerId peer1,
               PeerId peer2);

   public:
    StreamPair(std::shared_ptr<basic::Scheduler> post,
               bool is_initiator,
               PeerId peer_id)
        : post_{std::move(post)},
          is_initiator_{is_initiator},
          peer_id_{std::move(peer_id)} {}

    ~StreamPair() {
      reset();
    }

    void read(BytesOut out, size_t bytes, ReadCallbackFunc cb) override {
      ambigousSize(out, bytes);
      libp2p::readReturnSize(shared_from_this(), out, std::move(cb));
    }
    void readSome(BytesOut out, size_t bytes, ReadCallbackFunc cb) override {
      ambigousSize(out, bytes);
      if (out.empty()) {
        throw std::logic_error{"StreamPair::readSome zero bytes"};
      }
      if (read_buffer_.size() == 0) {
        if (read_closed_) {
          post(std::move(cb), make_error_code(boost::asio::error::eof));
          return;
        }
        reading_ = Reading{out, std::move(cb)};
        return;
      }
      auto n = std::min(read_buffer_.size(), out.size());
      memcpy(out.data(), read_buffer_.data().data(), n);
      read_buffer_.consume(n);
      post(std::move(cb), n);
    }
    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override {
      post(std::move(cb), res);
    }

    void writeSome(BytesIn in, size_t bytes, WriteCallbackFunc cb) override {
      ambigousSize(in, bytes);
      if (in.empty()) {
        throw std::logic_error{"StreamPair::writeSome zero bytes"};
      }
      if (auto writer = writer_.lock()) {
        writer->onRead(in);
        post(std::move(cb), in.size());
      } else {
        post(std::move(cb), std::errc::broken_pipe);
      }
    }
    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override {
      post(std::move(cb), ec);
    }

    bool isClosedForRead() const override {
      return read_buffer_.size() == 0 and read_closed_;
    }
    bool isClosedForWrite() const override {
      return writer_.expired();
    }
    bool isClosed() const override {
      return isClosedForRead() and isClosedForWrite();
    }
    void close(VoidResultHandlerFunc cb) override {
      if (auto writer = writer_.lock()) {
        writer->onReadClose();
      }
      onWriteClose();
      post(std::move(cb), outcome::success());
    }
    void reset() override {
      if (auto writer = writer_.lock()) {
        writer->onReadClose();
        writer->onWriteClose();
      }
      onReadClose();
      onWriteClose();
    }
    void adjustWindowSize(uint32_t, VoidResultHandlerFunc) override {}
    outcome::result<bool> isInitiator() const override {
      return is_initiator_;
    }
    outcome::result<PeerId> remotePeerId() const override {
      return peer_id_;
    }
    outcome::result<Multiaddress> localMultiaddr() const override {
      throw std::logic_error{"StreamPair::localMultiaddr"};
    }
    outcome::result<Multiaddress> remoteMultiaddr() const override {
      return Multiaddress::create(fmt::format("/p2p/{}", peer_id_.toBase58()))
          .value();
    }

   private:
    void post(auto cb, auto arg) {
      post_->schedule([cb{std::move(cb)}, arg{std::move(arg)}]() mutable {
        cb(std::move(arg));
      });
    }
    void onRead(BytesIn in) {
      if (auto reading = qtils::optionTake(reading_)) {
        auto n = std::min(in.size(), reading->out.size());
        memcpy(reading->out.data(), in.data(), n);
        in = in.subspan(n);
        post(std::move(reading->cb), n);
      }
      if (not in.empty()) {
        auto n = in.size();
        memcpy(read_buffer_.prepare(n).data(), in.data(), n);
        read_buffer_.commit(n);
      }
    }
    void onReadClose() {
      read_closed_ = true;
      if (auto reading = qtils::optionTake(reading_)) {
        post(std::move(reading->cb), make_error_code(boost::asio::error::eof));
      }
    }
    void onWriteClose() {
      writer_.reset();
    }

    struct Reading {
      BytesOut out;
      ReadCallbackFunc cb;
    };
    std::shared_ptr<basic::Scheduler> post_;
    bool is_initiator_;
    PeerId peer_id_;
    std::weak_ptr<StreamPair> writer_;
    std::optional<Reading> reading_;
    boost::asio::streambuf read_buffer_;
    bool read_closed_ = false;
  };

  std::pair<std::shared_ptr<Stream>, std::shared_ptr<Stream>> streamPair(
      std::shared_ptr<basic::Scheduler> post, PeerId peer1, PeerId peer2) {
    auto stream1 = std::make_shared<StreamPair>(post, true, peer2);
    auto stream2 = std::make_shared<StreamPair>(post, false, peer1);
    stream1->writer_ = stream2;
    stream2->writer_ = stream1;
    return {stream1, stream2};
  }
}  // namespace libp2p::connection
