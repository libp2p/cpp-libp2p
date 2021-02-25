/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include <libp2p/basic/write_queue.hpp>

namespace libp2p::basic {

  bool WriteQueue::canEnqueue(size_t size) const {
    return (size + total_unsent_size_ <= size_limit_);
  }

  size_t WriteQueue::unsentBytes() const {
    return total_unsent_size_;
  }

  void WriteQueue::enqueue(DataRef data, bool some,
                           Writer::WriteCallbackFunc cb) {
    auto data_sz = static_cast<size_t>(data.size());

    assert(data_sz > 0);
    assert(canEnqueue(data_sz));

    total_unsent_size_ += data_sz;
    queue_.push_back({data, 0, 0, data_sz, some, std::move(cb)});
  }

  size_t WriteQueue::dequeue(size_t window_size, DataRef &out, bool &some) {
    if (total_unsent_size_ == 0 || window_size == 0
        || active_index_ >= queue_.size()) {
      out = DataRef{};
      return window_size;
    }

    assert(!queue_.empty());

    auto &item = queue_[active_index_];

    assert(item.unacknowledged + item.acknowledged + item.unsent
           == static_cast<size_t>(item.data.size()));
    assert(item.unsent > 0);

    out = item.data.subspan(item.acknowledged + item.unacknowledged);
    auto sz = static_cast<size_t>(out.size());

    assert(sz == item.unsent);

    if (sz > window_size) {
      sz = window_size;
      out = out.subspan(0, window_size);
    }

    item.unsent -= sz;
    item.unacknowledged += sz;

    if (item.some) {
      assert(item.acknowledged == 0);
      some = true;
      ++active_index_;
    } else {
      some = false;
      if (item.unsent == 0) {
        ++active_index_;
      }
    }

    assert(item.unacknowledged + item.acknowledged + item.unsent
           == static_cast<size_t>(item.data.size()));

    assert(total_unsent_size_ >= sz);
    total_unsent_size_ -= sz;

    return window_size - sz;
  }

  bool WriteQueue::ack(size_t size) {
    if (queue_.empty() || size == 0) {
      return false;
    }

    auto &item = queue_.front();

    auto total_size = item.acknowledged + item.unacknowledged + item.unsent;

    assert(total_size == static_cast<size_t>(item.data.size()));

    if (size > item.unacknowledged) {
      return false;
    }

    bool completed = false;

    if (item.some) {
      completed = true;
      total_size = size;

    } else {
      item.unacknowledged -= size;
      item.acknowledged += size;

      completed = (item.acknowledged == total_size);
    }

    if (!completed) {
      assert(total_size > item.acknowledged);
      return true;
    }

    auto cb = std::move(item.cb);
    queue_.pop_front();
    if (queue_.empty()) {
      assert(total_unsent_size_ == 0);
      active_index_ = 0;
    } else if (active_index_ > 0) {
      --active_index_;
    }
    if (cb) {
      cb(total_size);
    }

    return true;
  }

  void WriteQueue::broadcast(const BroadcastFn &fn) {
    for (auto &item : queue_) {
      if (!item.cb) {
        continue;
      }
      if (!fn(std::move(item.cb))) {
        break;
      }
    }
  }

  void WriteQueue::clear() {
    active_index_ = 0;
    total_unsent_size_ = 0;
    std::deque<Data> tmp_queue;
    queue_.swap(tmp_queue);
  }

}  // namespace libp2p::basic
