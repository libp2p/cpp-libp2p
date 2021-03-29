/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>
#include <cstring>

#include <libp2p/basic/read_buffer.hpp>

namespace libp2p::basic {

  ReadBuffer::ReadBuffer(size_t alloc_granularity)
      : alloc_granularity_(alloc_granularity),
        total_size_(0),
        first_byte_offset_(0),
        capacity_remains_(0) {
    assert(alloc_granularity > 0);
  }

  void ReadBuffer::add(BytesRef bytes) {
    size_t sz = bytes.size();
    if (sz == 0) {
      return;
    }

    if (capacity_remains_ >= sz) {
      assert(!fragments_.empty());

      auto &vec = fragments_.back();
      vec.insert(vec.end(), bytes.begin(), bytes.end());

      capacity_remains_ -= sz;
    } else if (capacity_remains_ > 0) {
      auto &vec = fragments_.back();

      size_t new_capacity = vec.size() + sz + alloc_granularity_;
      vec.reserve(new_capacity);
      vec.insert(vec.end(), bytes.begin(), bytes.end());

      capacity_remains_ = alloc_granularity_;
    } else {
      fragments_.emplace_back();
      auto &vec = fragments_.back();

      size_t new_capacity = sz + alloc_granularity_;

      vec.reserve(new_capacity);
      vec.insert(vec.end(), bytes.begin(), bytes.end());

      capacity_remains_ = alloc_granularity_;
    }

    total_size_ += sz;
  }

  size_t ReadBuffer::consume(BytesRef &out) {
    if (empty()) {
      return 0;
    }

    size_t n_bytes = out.size();
    if (n_bytes >= total_size_) {
      return consumeAll(out);
    }

    auto remains = n_bytes;
    auto *p = out.data();

    while (remains > 0) {
      auto consumed = consumePart(p, remains);

      assert(consumed <= remains);

      remains -= consumed;
      p += consumed;  // NOLINT
    }

    total_size_ -= n_bytes;
    return n_bytes;
  }

  size_t ReadBuffer::addAndConsume(BytesRef in, BytesRef &out) {
    if (in.empty()) {
      return consume(out);
    }

    if (out.empty()) {
      add(in);
      return 0;
    }

    if (empty()) {
      if (in.size() <= out.size()) {
        memcpy(out.data(), in.data(), in.size());
        return in.size();
      }
      memcpy(out.data(), in.data(), out.size());
      in = in.subspan(out.size());
      add(in);
      return out.size();
    }

    auto out_size = static_cast<size_t>(out.size());
    size_t consumed = 0;

    if (out_size <= total_size_) {
      consumed = consume(out);
      add(in);
      return consumed;
    }

    consumed = consumeAll(out);
    auto out_remains = out.subspan(consumed);
    return consumed + addAndConsume(in, out_remains);
  }

  void ReadBuffer::clear() {
    total_size_ = 0;
    first_byte_offset_ = 0;
    capacity_remains_ = 0;
    std::deque<Fragment>{}.swap(fragments_);
  }

  size_t ReadBuffer::consumeAll(BytesRef &out) {
    assert(!fragments_.empty());
    auto *p = out.data();
    auto n = fragments_.front().size() - first_byte_offset_;
    assert(n <= fragments_.front().size());

    memcpy(p, fragments_.front().data() + first_byte_offset_, n);  // NOLINT

    auto it = ++fragments_.begin();
    while (it != fragments_.end()) {
      p += n;  // NOLINT
      n = it->size();
      memcpy(p, it->data(), n);
      ++it;
    }

    auto ret = total_size_;

    total_size_ = 0;
    first_byte_offset_ = 0;
    capacity_remains_ = 0;

    // Find one fragment if not too large to avoid further allocations
    bool keep_one_fragment = false;
    bool is_first = true;
    for (auto &f : fragments_) {
      if (f.capacity() <= alloc_granularity_ * 2) {
        f.clear();
        capacity_remains_ = f.capacity();
        if (!is_first) {
          fragments_.front() = std::move(f);
        }
        keep_one_fragment = true;
        break;
      }
      if (is_first) {
        is_first = false;
      }
    }
    fragments_.resize(keep_one_fragment ? 1 : 0);

    return ret;
  }

  size_t ReadBuffer::consumePart(uint8_t *out, size_t n) {
    if (fragments_.empty()) {
      return 0;
    }

    auto &f = fragments_.front();

    assert(f.size() > first_byte_offset_);

    auto fragment_size = f.size() - first_byte_offset_;
    if (n > fragment_size) {
      n = fragment_size;
    }

    memcpy(out, f.data() + first_byte_offset_, n);  // NOLINT

    if (n < fragment_size) {
      first_byte_offset_ += n;
    } else {
      first_byte_offset_ = 0;
      fragments_.pop_front();
    }

    return n;
  }

  FixedBufferCollector::FixedBufferCollector(size_t expected_size,
                                             size_t memory_threshold)
      : memory_threshold_(memory_threshold), expected_size_(expected_size) {
  }

  void FixedBufferCollector::expect(size_t size) {
    expected_size_ = size;
    buffer_.clear();
    auto reserved = buffer_.capacity();
    if ((reserved > memory_threshold_) && (expected_size_ < reserved * 3 / 4)) {
      Buffer new_buffer;
      buffer_.swap(new_buffer);
    }
  }

  boost::optional<FixedBufferCollector::CBytesRef>
  FixedBufferCollector::add(CBytesRef &data) {
    assert(expected_size_ >= buffer_.size());

    auto appending = static_cast<size_t>(data.size());
    auto buffered = buffer_.size();

    if (buffered == 0) {
      if (appending >= expected_size_) {
        // dont buffer, just split
        CBytesRef ret = data.subspan(0, expected_size_);
        data = data.subspan(expected_size_);
        expected_size_ = 0;
        return ret;
      }
      buffer_.reserve(expected_size_);
    }

    auto unread = expected_size_ - buffer_.size();
    if (unread == 0) {
      // didnt expect anything
      return boost::none;
    }

    bool filled = false;
    if (appending >= unread) {
      appending = unread;
      filled = true;
    }

    buffer_.insert(buffer_.end(), data.begin(), data.begin() + appending);
    data = data.subspan(appending);

    if (filled) {
      return CBytesRef(buffer_);
    }

    return boost::none;
  }

  boost::optional<FixedBufferCollector::BytesRef>
  FixedBufferCollector::add(BytesRef &data) {
    auto &span = (CBytesRef&)(data); //NOLINT
    auto ret = add(span);
    if (ret.has_value()) {
      auto& v = ret.value();
      return BytesRef((uint8_t*)v.data(), v.size()); // NOLINT
    }
    return boost::none;
  }

  void FixedBufferCollector::reset() {
    expected_size_ = 0;
    Buffer new_buffer;
    buffer_.swap(new_buffer);
  }

}  // namespace libp2p::basic
