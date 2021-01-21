/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_CONTENTROUTING
#define LIBP2P_PROTOCOL_KADEMLIA_CONTENTROUTING

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  // ContentRouting is used to find information about who has what content.
  class ContentRouting {
   public:
    virtual ~ContentRouting() = default;

    // Provide adds the given CID to the content routing system.
    // If 'true' is passed, it also announces it, otherwise it is just kept in
    // the local accounting of which objects are being provided.
    virtual outcome::result<void> provide(const Key &key, bool need_notify) = 0;

    // Search for peers who are able to provide a given key.
    virtual outcome::result<void> findProviders(
        const Key &key, size_t limit, FoundProvidersHandler handler) = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_CONTENTROUTING
