/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAD_MESSAGE_READ_WRITER_MOCK_HPP
#define KAGOME_KAD_MESSAGE_READ_WRITER_MOCK_HPP

#include "p2p/protocol/kademlia/message_read_writer.hpp"

#include <gmock/gmock.h>

namespace libp2p::protocol::kademlia {

  struct MessageReadWriterMock : public MessageReadWriter {
    ~MessageReadWriterMock() override = default;

    MOCK_METHOD3(findPeerSingle,
                 void(const Key &p, const peer::PeerId &id,
                      PeerInfosResultFunc f));
  };

}  // namespace libp2p::protocol::kademlia

#endif  // KAGOME_KAD_MESSAGE_READ_WRITER_MOCK_HPP
