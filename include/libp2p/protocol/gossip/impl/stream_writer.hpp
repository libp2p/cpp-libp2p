/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_STREAM_WRITER_HPP
#define LIBP2P_PROTOCOL_GOSSIP_STREAM_WRITER_HPP

#include <deque>
#include <functional>

#include <libp2p/connection/stream.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/gossip/impl/common.hpp>

namespace libp2p::protocol::gossip {

  /// Writes RPC messages to connected stream
  class StreamWriter : public std::enable_shared_from_this<StreamWriter> {
   public:
    /// Feedback interface from writer to its owning object (i.e. pub-sub
    /// server)
    using Feedback = std::function<void(const PeerContextPtr &from,
                                        outcome::result<Success> event)>;

    /// Ctor. N.B. StreamWriter instance cannot live longer than its creators
    /// by design, so dependencies are stored by reference.
    /// Also, peer is passed separately because it cannot be fetched from stream
    /// once the stream is dead
    StreamWriter(const Config &config, Scheduler &scheduler,
                 const Feedback &feedback,
                 std::shared_ptr<connection::Stream> stream,
                 PeerContextPtr peer);

    /// Writes an outgoing message to stream, if there is serialization error
    /// it will be posted in asynchronous manner
    void write(outcome::result<SharedBuffer> serialization_res);

    /// Closes writer and discards all outgoing messages
    void close();

   private:
    void onMessageWritten(outcome::result<size_t> res);
    void beginWrite(SharedBuffer buffer);
    void endWrite();
    void asyncPostError(Error error);

    const Scheduler::Ticks timeout_;
    Scheduler &scheduler_;
    const Feedback &feedback_;
    std::shared_ptr<connection::Stream> stream_;
    PeerContextPtr peer_;

    std::deque<SharedBuffer> pending_buffers_;

    /// Number of bytes being awaited in active wrote operation
    size_t writing_bytes_ = 0;

    // TODO(artem): limit pending bytes and close slow streams that way
    size_t pending_bytes_ = 0;

    /// Dont send feedback or schedule writes anymore
    bool closed_ = false;

    /// Handle for current operation timeout guard
    Scheduler::Handle timeout_handle_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_STREAM_WRITER_HPP
