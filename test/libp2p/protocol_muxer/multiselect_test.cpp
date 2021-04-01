/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect.hpp>
#include <libp2p/protocol_muxer/multiselect/parser.hpp>
#include <libp2p/protocol_muxer/multiselect/serializing.hpp>

#include <gtest/gtest.h>

#include "testutil/prepare_loggers.hpp"
/**
 * @given static vector
 * @when resizing it over static capacity
 * @then bad_alloc is thrown
 */
TEST(Multiselect, TmpBufThrows) {
  using libp2p::protocol_muxer::multiselect::detail::TmpMsgBuf;
  using libp2p::protocol_muxer::multiselect::kMaxMessageSize;
  TmpMsgBuf buf;
  buf.resize(kMaxMessageSize / 2);
  EXPECT_THROW(buf.resize(buf.capacity() + 1), std::bad_alloc);
}

TEST(Multiselect, SingleValidMessages) {
  using namespace libp2p::protocol_muxer::multiselect;

  std::vector<Message> messages({
                                    {Message::kRightProtocolVersion, "/multistream/1.0.0"},
                                    {Message::kRightProtocolVersion, "/multistream/1.0.1"},
                                    {Message::kRightProtocolVersion, "/multistream-select/0.4.0"},
                                    {Message::kWrongProtocolVersion, "/multistream/2.0.0"},
                                    {Message::kProtocolName, "/echo/1.0.0"},
                                    {Message::kNAMessage, "na"},
                                    {Message::kLSMessage, "ls"},
                                });

  detail::Parser reader;
  for (const auto &m : messages) {
    auto buf = detail::createMessage(m.content).value();
    EXPECT_GT(buf.size(), m.content.size());
    gsl::span<const uint8_t> span(buf);
    auto s = reader.consume(span);
    EXPECT_EQ(s, detail::Parser::kReady);
    EXPECT_EQ(reader.messages().size(), 1);
    const auto &received = reader.messages().front();
    EXPECT_EQ(received.content, m.content);
    EXPECT_EQ(received.type, m.type);
    reader.reset();
  }
}

TEST(Multiselect, SingleValidMessagesPartialRead) {
  using namespace libp2p::protocol_muxer::multiselect;

  std::vector<Message> messages({
                                    {Message::kRightProtocolVersion, "/multistream/1.0.0"},
                                    {Message::kRightProtocolVersion, "/multistream/1.0.1"},
                                    {Message::kRightProtocolVersion, "/multistream-select/0.4.0"},
                                    {Message::kWrongProtocolVersion, "/multistream/2.0.0"},
                                    {Message::kProtocolName, "/echo/1.0.0"},
                                    {Message::kNAMessage, "na"},
                                    {Message::kLSMessage, "ls"},
                                });

  using Span = gsl::span<const uint8_t>;

  auto split_span = [](Span span, size_t first_split,
                       size_t second_split) -> std::tuple<Span, Span, Span> {
    return {span.first(first_split),
            span.subspan(first_split, span.size() - second_split - first_split),
            span.last(second_split)};
  };

  auto test = [&](size_t first_split, size_t second_split) {
    detail::Parser reader;
    for (const auto &m : messages) {
      auto buf = detail::createMessage(m.content).value();
      EXPECT_GT(buf.size(), m.content.size());
      gsl::span<const uint8_t> span(buf);
      auto [s1, s2, s3] = split_span(span, first_split, second_split);
      auto s = reader.consume(s1);
      EXPECT_EQ(s, detail::Parser::kUnderflow);
      s = reader.consume(s2);
      EXPECT_EQ(s, detail::Parser::kUnderflow);
      s = reader.consume(s3);
      EXPECT_EQ(s, detail::Parser::kReady);
      EXPECT_EQ(reader.messages().size(), 1);
      const auto &received = reader.messages().front();
      EXPECT_EQ(received.content, m.content);
      EXPECT_EQ(received.type, m.type);
      reader.reset();
    }
  };

  test(1, 2);
  test(2, 1);
}
