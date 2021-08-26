/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_STREAM_HPP
#define LIBP2P_PROTOCOL_GOSSIP_STREAM_HPP

#include <deque>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/multi/uvarint.hpp>

#include "common.hpp"

namespace libp2p::protocol::gossip {

  class MessageReceiver;

  /// Reads/writes RPC messages from/to connected stream
  class Stream : public std::enable_shared_from_this<Stream> {
   public:
    /// Feedback interface to its owning object (i.e. pub-sub instance)
    using Feedback = std::function<void(PeerContextPtr from,
                                        outcome::result<Success> event)>;

    /// Ctor. N.B. Stream instance cannot live longer than its creators
    /// by design, so dependencies are stored by reference.
    /// Also, peer is passed separately because it cannot be fetched from stream
    /// once the stream is dead
    Stream(size_t stream_id, const Config &config, basic::Scheduler &scheduler,
           const Feedback &feedback, MessageReceiver &msg_receiver,
           std::shared_ptr<connection::Stream> stream, PeerContextPtr peer);

    /// Begins reading messages from stream
    void read();

    /// Writes an outgoing message to stream, if there is serialization error
    /// it will be posted in asynchronous manner
    void write(outcome::result<SharedBuffer> serialization_res);

    /// Closes the reader so that it will ignore further bytes from wire
    void close();

   private:
    void onLengthRead(outcome::result<multi::UVarint> varint);
    void onMessageRead(outcome::result<size_t> res);
    void beginWrite(SharedBuffer buffer);
    void onMessageWritten(outcome::result<size_t> res);
    void endWrite();
    void asyncPostError(Error error);

    [[maybe_unused]] const size_t stream_id_;
    const Time timeout_;
    basic::Scheduler &scheduler_;
    const size_t max_message_size_;
    const Feedback &feedback_;
    MessageReceiver &msg_receiver_;
    std::shared_ptr<connection::Stream> stream_;
    PeerContextPtr peer_;

    std::deque<SharedBuffer> pending_buffers_;

    /// Number of bytes being awaited in active wrote operation
    size_t writing_bytes_ = 0;

    // TODO(artem): limit pending bytes and close slow streams that way
    size_t pending_bytes_ = 0;

    std::shared_ptr<ByteArray> read_buffer_;
    /// Dont send feedback or schedule writes anymore
    bool closed_ = false;

    bool reading_ = false;

    /// Handle for current operation timeout guard
    basic::Scheduler::Handle timeout_handle_;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(libp2p::protocol::gossip::Stream);
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_STREAM_HPP
