/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "acceptance/p2p/host/protocol/client_test_session.hpp"

#include <gtest/gtest.h>
#include <boost/assert.hpp>
#include "libp2p/crypto/random_generator/boost_generator.hpp"

namespace libp2p::protocol {

  ClientTestSession::ClientTestSession(
      std::shared_ptr<connection::Stream> stream, size_t ping_times)
      : stream_(std::move(stream)),
        random_generator_{
            std::make_shared<crypto::random::BoostRandomGenerator>()},
        messages_left_{ping_times} {
    BOOST_ASSERT(stream_ != nullptr);
  }

  void ClientTestSession::handle(Callback cb) {
    write(std::move(cb));
  }

  void ClientTestSession::write(Callback cb) {
    if (messages_left_ == 0) {
      return;
    }

    --messages_left_;

    EXPECT_FALSE(stream_->isClosedForWrite()) << "stream is closed for write";

    write_buf_ = random_generator_->randomBytes(buffer_size_);

    stream_->write(write_buf_, buffer_size_,
                   [self = shared_from_this(),
                    cb{std::move(cb)}](outcome::result<size_t> rw) mutable {
                     if (!rw) {
                       return cb(rw.error());
                     }

                     self->read(cb);
                   });
  }

  void ClientTestSession::read(Callback cb) {
    EXPECT_FALSE(stream_->isClosedForRead()) << "stream is closed for read";

    read_buf_ = std::vector<uint8_t>(buffer_size_);

    stream_->read(read_buf_, buffer_size_,
                  [self = shared_from_this(),
                   cb{std::move(cb)}](outcome::result<size_t> rr) mutable {
                    if (!rr) {
                      return cb(rr.error());
                    }
                    cb(std::move(self->read_buf_));
                    return self->write(std::move(cb));
                  });
  }

}  // namespace libp2p::protocol
