/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/parser.hpp>

namespace libp2p::protocol_muxer::multiselect::detail {

  constexpr size_t kMaxRecursionDepth = 3;

  size_t Parser::bytesNeeded() const {
    size_t n = 0;
    if (state_ == kUnderflow) {
      // 1 is for varint reader...
      n = (expected_msg_size_ > 0) ? expected_msg_size_ : 1;
    }
    return n;
  }

  void Parser::reset() {
    messages_.clear();
    msg_buffer_.reset();
    state_ = kUnderflow;
    varint_reader_.reset();
    expected_msg_size_ = 0;
  }

  Parser::State Parser::consume(gsl::span<const uint8_t> &data) {
    static constexpr size_t kMaybeAverageMessageLength = 17;

    if (state_ == kReady) {
      return kError;
    }

    while (!data.empty()) {
      if (state_ != kUnderflow) {
        break;
      }

      if (expected_msg_size_ == 0) {
        auto s = varint_reader_.consume(data);
        if (s == VarintPrefixReader::kUnderflow) {
          continue;
        }
        if (s != VarintPrefixReader::kReady) {
          state_ = kOverflow;
          break;
        }
        expected_msg_size_ = varint_reader_.value();
        if (expected_msg_size_ == 0) {
          // zero varint received, not acceptable, but not fatal
          reset();
        } else {
          messages_.reserve(expected_msg_size_ / kMaybeAverageMessageLength);
          msg_buffer_.expect(expected_msg_size_);
        }
      } else {
        consumeData(data);
      }
    }

    return state_;
  }

  void Parser::consumeData(gsl::span<const uint8_t> &data) {
    assert(varint_reader_.state() == VarintPrefixReader::kReady);
    assert(expected_msg_size_ > 0);

    auto maybe_msg_ready = msg_buffer_.add(data);
    if (maybe_msg_ready) {
      readFinished(maybe_msg_ready.value());
    }
  }

  void Parser::readFinished(gsl::span<const uint8_t> msg) {
    assert(expected_msg_size_ == static_cast<size_t>(msg.size()));
    assert(expected_msg_size_ != 0);

    auto span2sv = [](gsl::span<const uint8_t> span) -> std::string_view {
      if (span.empty()) {
        return std::string_view();
      }
      return std::string_view((const char *)(span.data()),  // NOLINT
                              static_cast<size_t>(span.size()));
    };

    auto split = [this](std::string_view msg) {
      size_t first = 0;

      while (first < msg.size()) {
        auto second = msg.find(kNewLine, first);
        if (first != second) {
          messages_.push_back(
              {Message::kProtocolName, msg.substr(first, second - first)});
        }
        if (second == std::string_view::npos) {
          break;
        }
        first = second + 1;
      }
    };

    if (msg[expected_msg_size_ - 1] != kNewLine) {
      messages_.push_back({Message::kInvalidMessage, span2sv(msg)});
      state_ = kReady;
      return;
    }

    auto subspan = msg.first(expected_msg_size_ - 1);

    if (expected_msg_size_ > 1 && msg[expected_msg_size_ - 2] == kNewLine) {
      parseNestedMessages(subspan);
    } else {
      split(span2sv(subspan));
      processReceivedMessages();
    }

    assert(state_ != kUnderflow);
  }

  void Parser::parseNestedMessages(gsl::span<const uint8_t> &data) {
    if (recursion_depth_ == kMaxRecursionDepth) {
      state_ = kOverflow;
      return;
    }

    Parser nested_parser(recursion_depth_ + 1);

    while (!data.empty() && nested_parser.state_ == kUnderflow) {
      auto s = nested_parser.consume(data);
      if (s == kReady) {
        messages_.insert(messages_.end(), nested_parser.messages_.begin(),
                         nested_parser.messages_.end());
        nested_parser.reset();
      }
    }

    if (data.empty()) {
      state_ = kReady;
    } else {
      state_ = kError;
    }
  }

  void Parser::processReceivedMessages() {
    static constexpr std::string_view kThisProtocol("/multistream/1.");
    static constexpr std::string_view kCompatibleProtocol(
        "/multistream-select/0.");
    static constexpr std::string_view kProtocolPrefix("/multistream");

    auto starts_with = [](const std::string_view &x,
                          const std::string_view &y) -> bool {
      if (x.size() < y.size() || x.empty() || y.empty()) {
        return false;
      }
      return memcmp(x.data(), y.data(), y.size()) == 0;
    };

    bool first = true;
    for (auto &msg : messages_) {
      if (first) {
        first = false;
        if (starts_with(msg.content, kThisProtocol)
            || starts_with(msg.content, kCompatibleProtocol)) {
          msg.type = Message::kRightProtocolVersion;
          continue;
        }
        if (starts_with(msg.content, kProtocolPrefix)) {
          msg.type = Message::kWrongProtocolVersion;
          continue;
        }
      }
      if (msg.content == kNA) {
        msg.type = Message::kNAMessage;
      } else if (msg.content == kLS) {
        msg.type = Message::kLSMessage;
      }
    }

    state_ = kReady;
  }

}  // namespace libp2p::protocol_muxer::mutiselect::detail
