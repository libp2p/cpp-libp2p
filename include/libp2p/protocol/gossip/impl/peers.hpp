/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_PEERS_HPP
#define LIBP2P_PROTOCOL_GOSSIP_PEERS_HPP

#include <functional>
#include <set>

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/protocol/gossip/common.hpp>

namespace libp2p::protocol::gossip {

  /// Builds outbound message for wire protocol
  class MessageBuilder;

  /// Data related to peer needed by pub-sub protocols
  struct PeerContext {
    /// Used by shared ptr only
    using Ptr = std::shared_ptr<PeerContext>;

    /// The key
    const peer::PeerId peer_id;

    /// Set of topics this peer is subscribed to
    std::set<TopicId> subscribed_to;

    /// Builds message to be sent to this peer
    std::shared_ptr<MessageBuilder> message_to_send;

    /// Not null iff this peer can be dialed to
    boost::optional<multi::Multiaddress> dial_to;

    ~PeerContext() = default;
    PeerContext(PeerContext &&) = delete;
    PeerContext(const PeerContext &) = delete;
    PeerContext &operator=(const PeerContext &) = delete;
    PeerContext &operator=(PeerContext &&) = delete;

    explicit PeerContext(peer::PeerId id) : peer_id(std::move(id)) {}
  };

  /// Operators needed to place PeerContext::Ptr into PeerSet but use
  /// const peer::PeerId& as a key
  bool operator<(const PeerContext::Ptr &ctx, const peer::PeerId &peer);
  bool operator<(const peer::PeerId &peer, const PeerContext::Ptr &ctx);
  bool operator<(const PeerContext::Ptr &a, const PeerContext::Ptr &b);

  /// Peer set for pub-sub protocols
  class PeerSet {
   public:
    PeerSet() = default;
    ~PeerSet() = default;
    PeerSet(const PeerSet &) = default;
    PeerSet(PeerSet &&) = default;
    PeerSet &operator=(const PeerSet &) = default;
    PeerSet &operator=(PeerSet &&) = default;

    /// Finds peer context by id
    boost::optional<PeerContext::Ptr> find(const peer::PeerId &id) const;

    /// Inserts peer context into set, returns false if already inserted
    bool insert(PeerContext::Ptr ctx);

    /// Erases peer context from set, returns false not found
    bool erase(const peer::PeerId &id);

    /// Clears all data
    void clear();

    /// Returns true iff size() == 0
    bool empty() const;

    /// Returns # of peers in list
    size_t size() const;

    /// Selects up to n random peers
    std::vector<PeerContext::Ptr> selectRandomPeers(size_t n) const;

    /// Callback for peer selection
    using SelectCallback = std::function<void(const PeerContext::Ptr &)>;

    /// Callback for peer filtering
    using FilterCallback = std::function<bool(const PeerContext::Ptr &)>;

    /// Selects all peers
    void selectAll(const SelectCallback &callback) const;

    /// Selects peers filtered by external criteria
    void selectIf(const SelectCallback &callback,
                  const FilterCallback &filter) const;

    /// Conditionally erases peers from the set
    void eraseIf(const FilterCallback &filter);

   private:
    std::set<PeerContext::Ptr, std::less<>> peers_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_PEERS_HPP
