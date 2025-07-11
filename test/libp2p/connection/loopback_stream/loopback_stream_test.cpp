/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/basic/write_return_size.hpp>
#include <libp2p/connection/loopback_stream.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include <qtils/test/outcome.hpp>
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
  ASSERT_OUTCOME_SUCCESS(hash,
                         Multihash::create(libp2p::multi::sha256, kBuffer));
  ASSERT_OUTCOME_SUCCESS(peer_id,
                         PeerId::fromBase58(encodeBase58(hash.toBuffer())));

  std::shared_ptr<libp2p::connection::Stream> stream =
      std::make_shared<LoopbackStream>(PeerInfo{peer_id, {}}, context);
  bool all_executed{false};
  libp2p::writeReturnSize(
      stream, kBuffer, [stream, buf = kBuffer, &all_executed](auto result) {
        ASSERT_OUTCOME_SUCCESS(bytes, result);
        ASSERT_EQ(bytes, kBufferSize);
        auto read_buf = std::make_shared<Buffer>(kBufferSize, 0);
        ASSERT_EQ(read_buf->size(), kBufferSize);
        ASSERT_NE(*read_buf, buf);
        libp2p::readReturnSize(
            stream,
            *read_buf,
            [buf = read_buf, source_buf = buf, &all_executed](auto result) {
              ASSERT_OUTCOME_SUCCESS(bytes, result);
              ASSERT_EQ(bytes, kBufferSize);
              ASSERT_EQ(*buf, source_buf);
              all_executed = true;
            });
      });
  context->run();
  ASSERT_TRUE(all_executed);
}
