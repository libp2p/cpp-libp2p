/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/connection/loopback_stream.hpp>

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>  // clang-format puts it here, sorry ¯\_(ツ)_/¯
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include <testutil/outcome.hpp>
#include <testutil/prepare_loggers.hpp>

using libp2p::connection::LoopbackStream;
using libp2p::crypto::Buffer;
using libp2p::multi::Multihash;
using libp2p::multi::sha256;
using libp2p::multi::detail::encodeBase58;
using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;

class LoopbackStreamTest : public testing::Test {
 public:
  static constexpr size_t kBufferSize = 43;
  const Buffer kBuffer = Buffer(kBufferSize, 1);
  std::shared_ptr<boost::asio::io_context> context;

  void SetUp() override {
    context = std::make_shared<boost::asio::io_context>();
  };

  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }
};

/**
 * @given a loopback stream
 * @when a data is written into the stream
 * @then exactly the same data can be read from the stream
 */
TEST_F(LoopbackStreamTest, Basic) {
  EXPECT_OUTCOME_TRUE(hash, Multihash::create(libp2p::multi::sha256, kBuffer));
  EXPECT_OUTCOME_TRUE(peer_id,
                      PeerId::fromBase58(encodeBase58(hash.toBuffer())))

  std::shared_ptr<libp2p::connection::Stream> stream =
      std::make_shared<LoopbackStream>(PeerInfo{peer_id, {}}, context);
  bool all_executed{false};
  stream->write(kBuffer, kBuffer.size(),
                [stream, buf = kBuffer, &all_executed](auto result) {
                  EXPECT_OUTCOME_TRUE(bytes, result);
                  ASSERT_EQ(bytes, kBufferSize);
                  auto read_buf = std::make_shared<Buffer>(kBufferSize, 0);
                  ASSERT_EQ(read_buf->size(), kBufferSize);
                  ASSERT_NE(*read_buf, buf);
                  stream->read(*read_buf, kBufferSize,
                               [buf = read_buf, source_buf = buf,
                                &all_executed](auto result) {
                                 EXPECT_OUTCOME_TRUE(bytes, result);
                                 ASSERT_EQ(bytes, kBufferSize);
                                 ASSERT_EQ(*buf, source_buf);
                                 all_executed = true;
                               });
                });
  context->run();
  ASSERT_TRUE(all_executed);
}
