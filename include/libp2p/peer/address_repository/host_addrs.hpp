/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>

#include <libp2p/network/listener_manager.hpp>
#include <libp2p/peer/identity_manager.hpp>

namespace libp2p {
  class HostAddrs {
   public:
    struct Result {
      void add(Multiaddress addr) {
        for (auto &[p, v] : addr.getProtocolsWithValues()) {
          if (p.code == multi::Protocol::Code::IP4) {
            if (v == "0.0.0.0") {
              return;
            }
          } else if (p.code == multi::Protocol::Code::IP6) {
            if (v == "::") {
              return;
            }
          }
        }
        set.emplace(std::move(addr));
      }
      std::vector<Multiaddress> asVec() && {
        return {std::move_iterator{set.begin()}, std::move_iterator{set.end()}};
      }
      std::unordered_set<Multiaddress> set;
    };

    HostAddrs(const peer::IdentityManager &id_mgr,
              std::shared_ptr<network::ListenerManager> listener)
        : peer_id_{id_mgr.getId()}, listener_{std::move(listener)} {}

    const PeerId &peerId() const {
      return peer_id_;
    }

    Result get() const {
      Result result;
      for (auto &addr : listener_->getListenAddresses()) {
        result.add(std::move(addr));
      }
      for (auto &addr : listener_->getListenAddressesInterfaces()) {
        result.add(std::move(addr));
      }
      return result;
    }

   private:
    PeerId peer_id_;
    std::shared_ptr<network::ListenerManager> listener_;
  };
}  // namespace libp2p
