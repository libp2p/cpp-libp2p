/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/message_manager.hpp>

#include <sstream>
#include <string_view>

#include <boost/algorithm/string/predicate.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/common/types.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer, MessageManager::ParseError,
                            e) {
  using Error = libp2p::protocol_muxer::MessageManager::ParseError;
  switch (e) {
    case Error::VARINT_IS_EXPECTED:
      return "expected varint, but not found";
    case Error::MSG_LENGTH_IS_INCORRECT:
      return "incorrect message length";
    case Error::MSG_IS_ILL_FORMED:
      return "format of the message does not meet the protocol spec";
  }
  return "unknown error";
}

namespace {
  using libp2p::common::ByteArray;
  using libp2p::multi::UVarint;
  using libp2p::outcome::result;
  using MultiselectMessage =
      libp2p::protocol_muxer::MessageManager::MultiselectMessage;

  /// string of ls message
  constexpr std::string_view kLsString = "ls\n";

  /// string of na message
  constexpr std::string_view kNaString = "na\n";

  /// ls message, ready to be sent
  const ByteArray kLsMsg = []() -> ByteArray {
    auto vec = UVarint{kLsString.size()}.toVector();
    vec.insert(vec.end(), kLsString.begin(), kLsString.end());
    return vec;
  }();

  /// na message, ready to be sent
  const ByteArray kNaMsg = []() -> ByteArray {
    auto vec = UVarint{kNaString.size()}.toVector();
    vec.insert(vec.end(), kNaString.begin(), kNaString.end());
    return vec;
  }();

  /**
   * Retrieve a varint from the line
   * @param line to be seeked
   * @return varint, if it was retrieved; error otherwise
   */
  result<UVarint> getVarint(std::string_view line) {
    using ParseError = libp2p::protocol_muxer::MessageManager::ParseError;

    if (line.empty()) {
      return ParseError::VARINT_IS_EXPECTED;
    }

    auto varint_opt = UVarint::create(gsl::make_span(
        reinterpret_cast<const uint8_t *>(line.data()),  // NOLINT
        reinterpret_cast<const uint8_t *>(line.data())   // NOLINT
            + line.size()));                             // NOLINT
    if (!varint_opt) {
      return ParseError::VARINT_IS_EXPECTED;
    }

    return *varint_opt;
  }

  /**
   * Get a protocol from a string of format <protocol><\n>
   * @param msg of the specified format
   * @return pure protocol string with \n thrown away
   */
  result<std::string> parseProtocolLine(std::string_view msg) {
    using ParseError = libp2p::protocol_muxer::MessageManager::ParseError;

    auto new_line_byte = msg.find('\n');
    if (new_line_byte == std::string_view::npos) {
      return ParseError::MSG_IS_ILL_FORMED;
    }

    return std::string{msg.substr(0, new_line_byte)};
  }

  /**
   * Get a protocol from a string of format <length-varint><protocol>
   * @param line of the specified format
   * @return pure protocol with varint and \n thrown away
   */
  result<std::string> parseProtocolsLine(std::string_view line) {
    using ParseError = libp2p::protocol_muxer::MessageManager::ParseError;

    auto varint_res = getVarint(line);
    if (!varint_res) {
      return ParseError::VARINT_IS_EXPECTED;
    }
    auto varint = std::move(varint_res.value());

    if (line.size() != varint.toUInt64()) {
      return ParseError::MSG_LENGTH_IS_INCORRECT;
    }

    return std::string{line.substr(varint.size())};
  }
}  // namespace

namespace libp2p::protocol_muxer {
  using MultiselectMessage = MessageManager::MultiselectMessage;

  outcome::result<MultiselectMessage> MessageManager::parseConstantMsg(
      gsl::span<const uint8_t> bytes) {
    // first varint is already read
    static constexpr std::string_view kLsMsgHex{"6C730A"};  // 'ls\n'
    static constexpr std::string_view kNaMsgHex{"6E610A"};  // 'na\n'
    static constexpr int64_t kConstMsgsLength{kLsMsgHex.size() / 2};

    if (bytes.size() == kConstMsgsLength) {
      auto msg_hex = common::hex_upper(bytes);
      if (msg_hex == kLsMsgHex) {
        return MultiselectMessage{MultiselectMessage::MessageType::LS};
      }
      if (msg_hex == kNaMsgHex) {
        return MultiselectMessage{MultiselectMessage::MessageType::NA};
      }
    }
    return ParseError::MSG_IS_ILL_FORMED;
  }

  outcome::result<MultiselectMessage> MessageManager::parseProtocols(
      gsl::span<const uint8_t> bytes) {
    MultiselectMessage message{MultiselectMessage::MessageType::PROTOCOLS};

    // each protocol is prepended with  a varint length and appended with '\n'
    std::string msg_str{bytes.data(), bytes.data() + bytes.size()};
    std::istringstream msg_stream{msg_str};

    std::string current_protocol;
    while (getline(msg_stream, current_protocol, '\n')) {
      if (current_protocol.empty()) {
        // it is a last iteration of the loop, as the message ends with two \n
        continue;
      }
      OUTCOME_TRY(parsed_protocol, parseProtocolsLine(current_protocol));
      message.protocols.push_back(std::move(parsed_protocol));
    }

    return message;
  }

  outcome::result<std::string> MessageManager::parseProtocol(
      gsl::span<const uint8_t> bytes) {
    if (bytes.empty()) {
      return ParseError::MSG_LENGTH_IS_INCORRECT;
    }
    return parseProtocolLine(
        std::string{bytes.data(), bytes.data() + bytes.size()});  // NOLINT
  }

  ByteArray MessageManager::openingMsg() {
    ByteArray buffer = multi::UVarint{kMultiselectHeader.size()}.toVector();
    buffer.insert(buffer.end(), kMultiselectHeader.begin(),
                  kMultiselectHeader.end());
    return buffer;
  }

  ByteArray MessageManager::lsMsg() {
    return kLsMsg;
  }

  ByteArray MessageManager::naMsg() {
    return kNaMsg;
  }

  ByteArray MessageManager::protocolMsg(const peer::Protocol &protocol) {
    ByteArray buffer = multi::UVarint{protocol.size() + 1}.toVector();
    buffer.insert(buffer.end(), std::make_move_iterator(protocol.begin()),
                  std::make_move_iterator(protocol.end()));
    buffer.push_back('\n');
    return buffer;
  }

  ByteArray MessageManager::protocolsMsg(
      gsl::span<const peer::Protocol> protocols) {
    ByteArray msg{};

    // insert protocols
    for (const auto &protocol : protocols) {
      auto buffer = protocolMsg(protocol);
      msg.insert(msg.end(), std::make_move_iterator(buffer.begin()),
                 std::make_move_iterator(buffer.end()));
    }
    msg.push_back('\n');

    // insert protocols section's size
    auto varint_protos_length = multi::UVarint{msg.size()}.toVector();
    msg.insert(msg.begin(),
               std::make_move_iterator(varint_protos_length.begin()),
               std::make_move_iterator(varint_protos_length.end()));

    return msg;
  }
}  // namespace libp2p::protocol_muxer
