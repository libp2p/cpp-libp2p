/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_PEERS_HPP
#define LIBP2P_PROTOCOL_GOSSIP_PEERS_HPP

#include <functional>
#include <set>

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/protocol/gossip/impl/common.hpp>

namespace libp2p::protocol::gossip {

  class MessageBuilder;
  class StreamWriter;
  class StreamReader;

  /// Data related to peer needed by pub-sub protocols
  struct PeerContext {
    /// The key
    const peer::PeerId peer_id;

    /// Set of topics this peer is subscribed to
    std::set<TopicId> subscribed_to;

    /// Builds message to be sent to this peer
    std::shared_ptr<MessageBuilder> message_to_send;

    /// Network stream writer
    std::shared_ptr<StreamWriter> writer;

    /// Network stream reader
    std::shared_ptr<StreamReader> reader;

    /// Not null iff this peer can be dialed to
    boost::optional<multi::Multiaddress> dial_to;

    /// Dialing to this peer is banned until this timestamp
    Time banned_until = 0;

    ~PeerContext() = default;
    PeerContext(PeerContext &&) = delete;
    PeerContext(const PeerContext &) = delete;
    PeerContext &operator=(const PeerContext &) = delete;
    PeerContext &operator=(PeerContext &&) = delete;

    explicit PeerContext(peer::PeerId id) : peer_id(std::move(id)) {}
  };

  /// Operators needed to place PeerContextPtr into PeerSet but use
  /// const peer::PeerId& as a key
  bool operator<(const PeerContextPtr &ctx, const peer::PeerId &peer);
  bool operator<(const peer::PeerId &peer, const PeerContextPtr &ctx);
  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b);

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
    boost::optional<PeerContextPtr> find(const peer::PeerId &id) const;

    /// Returns if the set contains such a peer
    bool contains(const peer::PeerId &id) const;

    /// Inserts peer context into set, returns false if already inserted
    bool insert(PeerContextPtr ctx);

    /// Removes peer context from set, returns erased item if found and erased
    boost::optional<PeerContextPtr> erase(const peer::PeerId &id);

    /// Clears all data
    void clear();

    /// Returns true iff size() == 0
    bool empty() const;

    /// Returns # of peers in list
    size_t size() const;

    /// Selects up to n random peers
    std::vector<PeerContextPtr> selectRandomPeers(size_t n) const;

    /// Callback for peer selection
    using SelectCallback = std::function<void(const PeerContextPtr &)>;

    /// Callback for peer filtering
    using FilterCallback = std::function<bool(const PeerContextPtr &)>;

    /// Selects all peers
    void selectAll(const SelectCallback &callback) const;

    /// Selects peers filtered by external criteria
    void selectIf(const SelectCallback &callback,
                  const FilterCallback &filter) const;

    /// Conditionally erases peers from the set
    void eraseIf(const FilterCallback &filter);

   private:
    std::set<PeerContextPtr, std::less<>> peers_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_PEERS_HPP
