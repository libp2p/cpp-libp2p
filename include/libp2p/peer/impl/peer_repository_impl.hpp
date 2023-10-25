/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_repository.hpp>

namespace libp2p::peer {

  class PeerRepositoryImpl : public PeerRepository {
   public:
    PeerRepositoryImpl(std::shared_ptr<AddressRepository> addrRepo,
                       std::shared_ptr<KeyRepository> keyRepo,
                       std::shared_ptr<ProtocolRepository> protocolRepo);

    AddressRepository &getAddressRepository() override;

    KeyRepository &getKeyRepository() override;

    ProtocolRepository &getProtocolRepository() override;

    std::unordered_set<PeerId> getPeers() const override;

    PeerInfo getPeerInfo(const PeerId &peer_id) const override;

   private:
    std::shared_ptr<AddressRepository> addr_;
    std::shared_ptr<KeyRepository> key_;
    std::shared_ptr<ProtocolRepository> proto_;
  };

}  // namespace libp2p::peer
