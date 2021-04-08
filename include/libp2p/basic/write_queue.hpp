/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_WRITE_QUEUE_HPP
#define LIBP2P_BASIC_WRITE_QUEUE_HPP

#include <deque>
#include <vector>

#include <libp2p/basic/writer.hpp>

namespace libp2p::basic {

  class WriteQueue {
   public:
    using DataRef = gsl::span<const uint8_t>;

    static constexpr size_t kDefaultSizeLimit = 64 * 1024 * 1024;

    explicit WriteQueue(size_t size_limit = kDefaultSizeLimit)
        : size_limit_(size_limit) {}

    /// Returns false if size will overflow the buffer
    bool canEnqueue(size_t size) const;

    /// Returns bytes enqueued and not yet sent
    size_t unsentBytes() const;

    /// Enqueues data
    void enqueue(DataRef data, bool some, basic::Writer::WriteCallbackFunc cb);

    /// Returns new window size
    size_t dequeue(size_t window_size, DataRef &out, bool &some);

    struct AckResult {
      // callback to be called to ack data was sent
      Writer::WriteCallbackFunc cb;

      // size to acknowledge, may differ from ack()'s size arg
      size_t size_to_ack = 0;

      // set to false if invalid arg or inconsistency
      bool data_consistent = true;
    };

    /// Calls write callback if full message was sent (or some),
    /// returns callback to ack + true if ok or false on inconsistency
    [[nodiscard]] AckResult ackDataSent(size_t size);

    /// Needed to broadcast error code to write callbacks
    [[nodiscard]] std::vector<Writer::WriteCallbackFunc> getAllCallbacks();

    /// Deallocates memory
    void clear();

   private:
    /// Data item w/callback
    struct Data {
      // data reference
      gsl::span<const uint8_t> data;

      // allow to send large messages partially
      size_t acknowledged;

      // was sent during write operation, not acknowledged yet
      size_t unacknowledged;

      // remaining bytes to dequeue
      size_t unsent;

      // allows to send at least 1 byte to complete operation
      bool some;

      // callback
      basic::Writer::WriteCallbackFunc cb;
    };

    size_t size_limit_;
    size_t active_index_ = 0;
    size_t total_unsent_size_ = 0;
    std::deque<Data> queue_;
  };

}  // namespace libp2p::basic

#endif  // LIBP2P_BASIC_WRITE_QUEUE_HPP
