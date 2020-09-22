/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_STREAM_CTX_HPP
#define LIBP2P_PROTOCOL_GOSSIP_STREAM_CTX_HPP

#include <functional>

#include <libp2p/connection/stream.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/gossip/impl/common.hpp>

namespace libp2p::protocol::gossip {

  class MessageReceiver;

  /// Reads and writes RPC messages from/to connected stream
  class StreamCtx : public std::enable_shared_from_this<StreamCtx> {
   public:
    /// Feedback interface from reader to its owning object (i.e. pub-sub
    /// server)
    using Feedback = std::function<void(const PeerContextPtr &from,
                                        outcome::result<Success> event)>;

    /// Ctor. N.B. StreamReader instance cannot live longer than its creators
    /// by design, so dependencies are stored by reference.
    /// Also, peer is passed separately because it cannot be fetched from stream
    /// once the stream is dead
    StreamCtx(const Config &config, Scheduler &scheduler,
                 const Feedback &feedback, MessageReceiver &msg_receiver,
                 std::shared_ptr<connection::Stream> stream,
                 PeerContextPtr peer);

    /// Reads an incoming message from stream
    void read();

    /// Closes the reader so that it will ignore further bytes from wire
    void close();

   private:
    void onLengthRead(boost::optional<multi::UVarint> varint_opt);
    void onMessageRead(outcome::result<size_t> res);
    void beginRead();
    void endRead();

    const Scheduler::Ticks timeout_;
    Scheduler &scheduler_;
    const size_t max_message_size_;
    const Feedback &feedback_;
    MessageReceiver &msg_receiver_;
    std::shared_ptr<connection::Stream> stream_;
    PeerContextPtr peer_;

    std::shared_ptr<ByteArray> buffer_;
    bool reading_ = false;

    /// Handle for current operation timeout guard
    Scheduler::Handle timeout_handle_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_STREAM_CTX_HPP
