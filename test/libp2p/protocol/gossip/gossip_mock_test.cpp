/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <generated/protocol/gossip/protobuf/rpc.pb.h>
#include <gtest/gtest.h>
#include <libp2p/basic/message_read_writer_uvarint.hpp>
#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/connection/stream_pair.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>
#include <qtils/bytestr.hpp>

#include "mock/libp2p/crypto/crypto_provider.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "mock/libp2p/peer/address_repository_mock.hpp"
#include "mock/libp2p/peer/identity_manager_mock.hpp"
#include "mock/libp2p/peer/peer_repository_mock.hpp"
#include "testutil/libp2p/peer.hpp"
#include "testutil/prepare_loggers.hpp"

using libp2p::Bytes;
using libp2p::BytesIn;
using libp2p::HostMock;
using libp2p::PeerId;
using libp2p::PeerInfo;
using libp2p::ProtocolName;
using libp2p::ProtocolPredicate;
using libp2p::StreamAndProtocol;
using libp2p::StreamAndProtocolCb;
using libp2p::StreamAndProtocolOrErrorCb;
using libp2p::StreamProtocols;
using libp2p::basic::ManualSchedulerBackend;
using libp2p::basic::MessageReadWriterUvarint;
using libp2p::basic::SchedulerImpl;
using libp2p::connection::Stream;
using libp2p::crypto::CryptoProviderMock;
using libp2p::crypto::marshaller::KeyMarshallerMock;
using libp2p::peer::AddressRepositoryMock;
using libp2p::peer::IdentityManagerMock;
using libp2p::peer::PeerRepositoryMock;
using libp2p::protocol::gossip::Gossip;
using libp2p::protocol::gossip::PeerKind;
using libp2p::protocol::gossip::TopicId;
using testing::_;
using testing::Return;
using testing::ReturnRef;
using testutil::randomPeerId;

using MessageIds = std::vector<uint8_t>;

inline Bytes encodeMessageId(uint8_t i) {
  return Bytes{i};
}
inline uint8_t decodeMessageId(std::string_view data) {
  return data.at(0);
}

struct Received {
  std::vector<bool> subscriptions{};
  MessageIds messages{};
  std::vector<bool> graft{};
  MessageIds ihave{};
  MessageIds iwant{};
  MessageIds idontwant{};
};

struct MockPeer {
  MockPeer(PeerId peer_id, ProtocolName version, std::shared_ptr<Stream> writer)
      : peer_id_{std::move(peer_id)},
        version_{std::move(version)},
        writer_{std::move(writer)},
        framing_{std::make_shared<MessageReadWriterUvarint>(writer_)} {}

  void expect(Received expected) {
    EXPECT_EQ(received_.subscriptions, expected.subscriptions);
    EXPECT_EQ(received_.messages, expected.messages);
    EXPECT_EQ(received_.graft, expected.graft);
    EXPECT_EQ(received_.ihave, expected.ihave);
    EXPECT_EQ(received_.iwant, expected.iwant);
    EXPECT_EQ(received_.idontwant, expected.idontwant);
    received_ = {};
  }

  void write(const auto &f) {
    pubsub::pb::RPC rpc;
    f(rpc);
    Bytes buffer;
    buffer.resize(rpc.ByteSizeLong());
    rpc.SerializeToArray(buffer.data(), buffer.size());
    framing_->write(
        buffer, [](outcome::result<size_t> r) { EXPECT_TRUE(r.has_value()); });
  }

  PeerId peer_id_;
  ProtocolName version_;
  std::shared_ptr<Stream> writer_;
  std::shared_ptr<MessageReadWriterUvarint> framing_;
  Received received_;
};

struct GossipMockTest : testing::Test {
  PeerId gossip_peer_id_{randomPeerId()};
  TopicId topic1{"topic1"};
  std::unordered_map<PeerId, std::shared_ptr<MockPeer>> peers_;
  libp2p::protocol::gossip::Config config_;
  std::optional<StreamAndProtocolCb> host_handler_;
  std::shared_ptr<ManualSchedulerBackend> scheduler_backend_ =
      std::make_shared<ManualSchedulerBackend>();
  std::shared_ptr<SchedulerImpl> scheduler_ = std::make_shared<SchedulerImpl>(
      scheduler_backend_, SchedulerImpl::Config{});
  libp2p::event::Bus bus_;
  std::shared_ptr<PeerRepositoryMock> peer_repo_ =
      std::make_shared<PeerRepositoryMock>();
  std::shared_ptr<AddressRepositoryMock> address_repo_ =
      std::make_shared<AddressRepositoryMock>();
  std::shared_ptr<HostMock> host_ = std::make_shared<HostMock>();
  std::shared_ptr<IdentityManagerMock> idmgr_ =
      std::make_shared<IdentityManagerMock>();
  std::shared_ptr<CryptoProviderMock> crypto_provider_ =
      std::make_shared<CryptoProviderMock>();
  std::shared_ptr<KeyMarshallerMock> key_marshaller_ =
      std::make_shared<KeyMarshallerMock>();
  std::shared_ptr<Gossip> gossip_;

  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    config_.D_min = 1;
    config_.D = 1;
    config_.D_lazy = 1;
    config_.flood_publish = false;
  }

  void setup() {
    EXPECT_CALL(*host_, getBus()).WillRepeatedly(ReturnRef(bus_));
    EXPECT_CALL(*host_, getPeerRepository())
        .WillRepeatedly(ReturnRef(*peer_repo_));
    EXPECT_CALL(*peer_repo_, getAddressRepository())
        .WillRepeatedly(ReturnRef(*address_repo_));
    EXPECT_CALL(*address_repo_, updateAddresses(_, _))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*host_, getPeerInfo())
        .WillRepeatedly(Return(PeerInfo{gossip_peer_id_, {}}));
    EXPECT_CALL(*host_, setProtocolHandler(_, _, _))
        .WillRepeatedly([&](StreamProtocols,
                            StreamAndProtocolCb cb,
                            ProtocolPredicate) { host_handler_ = cb; });
    EXPECT_CALL(*host_, newStream(_, _, _))
        .WillRepeatedly([&](const PeerInfo &info,
                            StreamProtocols,
                            StreamAndProtocolOrErrorCb cb) {
          auto &peer_id = info.id;
          auto peer = peers_.at(peer_id);
          auto [stream1, stream2] = libp2p::connection::streamPair(
              scheduler_, peer_id, gossip_peer_id_);
          read(peer, std::make_shared<MessageReadWriterUvarint>(stream2));
          scheduler_->schedule([cb, stream1, peer] {
            cb(StreamAndProtocol{stream1, peer->version_});
          });
        });
    gossip_ = libp2p::protocol::gossip::create(
        scheduler_, host_, idmgr_, crypto_provider_, key_marshaller_, config_);
    gossip_->setMessageIdFn(
        [](BytesIn from, BytesIn seqno, Bytes data) { return data; });
    gossip_->start();
  }

  void TearDown() override {
    for (auto &p : peers_) {
      p.second->expect({});
    }
  }

  auto connect(PeerKind peer_kind) {
    auto version_it =
        std::ranges::find_if(config_.protocol_versions,
                             [&](auto &p) { return p.second == peer_kind; });
    if (version_it == config_.protocol_versions.end()) {
      throw std::logic_error{"PeerKind"};
    }
    auto &version = version_it->first;
    auto peer_id = randomPeerId();
    auto [stream1, stream2] =
        libp2p::connection::streamPair(scheduler_, peer_id, gossip_peer_id_);
    auto peer = std::make_shared<MockPeer>(peer_id, version, stream1);
    peers_.emplace(peer_id, peer);
    scheduler_->schedule([this, stream2, version] {
      host_handler_.value()({stream2, version});
    });
    scheduler_backend_->callDeferred();
    return peer;
  }

  void read(std::shared_ptr<MockPeer> peer,
            std::shared_ptr<MessageReadWriterUvarint> framing) {
    framing->read([this, peer, framing](
                      outcome::result<std::shared_ptr<Bytes>> frame_res) {
      if (frame_res.has_error()) {
        return;
      }
      auto &frame = frame_res.value();
      pubsub::pb::RPC rpc;
      EXPECT_TRUE(rpc.ParseFromArray(frame->data(), frame->size()));
      for (auto &sub : rpc.subscriptions()) {
        EXPECT_EQ(sub.topicid(), topic1);
        peer->received_.subscriptions.emplace_back(sub.subscribe());
      }
      for (auto &pub : rpc.publish()) {
        peer->received_.messages.emplace_back(decodeMessageId(pub.data()));
      }
      for (auto &graft : rpc.control().graft()) {
        EXPECT_EQ(graft.topicid(), topic1);
        peer->received_.graft.emplace_back(true);
      }
      EXPECT_EQ(rpc.control().prune().size(), 0);
      for (auto &ihave : rpc.control().ihave()) {
        EXPECT_EQ(ihave.topicid(), topic1);
        for (auto &id : ihave.messageids()) {
          peer->received_.ihave.emplace_back(decodeMessageId(id));
        }
      }
      for (auto &iwant : rpc.control().iwant()) {
        for (auto &id : iwant.messageids()) {
          peer->received_.iwant.emplace_back(decodeMessageId(id));
        }
      }
      for (auto &idontwant : rpc.control().idontwant()) {
        for (auto &id : idontwant.message_ids()) {
          peer->received_.idontwant.emplace_back(decodeMessageId(id));
        }
      }
      read(peer, framing);
    });
  }

  auto subscribe() {
    auto sub = gossip_->subscribe({topic1}, [](Gossip::SubscriptionData) {});
    scheduler_backend_->callDeferred();
    for (auto &p : peers_) {
      p.second->expect({.subscriptions = {true}});
    }
    return sub;
  }

  void subscribe(MockPeer &peer, bool subscribe) {
    peer.write([&](pubsub::pb::RPC &rpc) {
      auto *sub = rpc.add_subscriptions();
      sub->set_topicid(topic1);
      sub->set_subscribe(subscribe);
    });
    scheduler_backend_->callDeferred();
  }

  void publish(uint8_t i) {
    gossip_->publish(topic1, {i});
    scheduler_backend_->callDeferred();
  }

  void publish(MockPeer &peer,
               uint8_t i,
               std::optional<PeerId> author = std::nullopt) {
    peer.write([&](pubsub::pb::RPC &rpc) {
      auto *pub = rpc.add_publish();
      *pub->mutable_from() =
          qtils::byte2str(author.value_or(peer.peer_id_).toVector());
      pub->set_seqno("");
      pub->set_topic(topic1);
      *pub->mutable_data() = qtils::byte2str(encodeMessageId(i));
    });
    scheduler_backend_->callDeferred();
  }

  void ihave(MockPeer &peer, uint8_t i) {
    peer.write([&](pubsub::pb::RPC &rpc) {
      auto *ihave = rpc.mutable_control()->add_ihave();
      ihave->set_topicid(topic1);
      *ihave->add_messageids() = qtils::byte2str(encodeMessageId(i));
    });
    scheduler_backend_->callDeferred();
  }

  void iwant(MockPeer &peer, uint8_t i) {
    peer.write([&](pubsub::pb::RPC &rpc) {
      auto *iwant = rpc.mutable_control()->add_iwant();
      *iwant->add_messageids() = qtils::byte2str(encodeMessageId(i));
    });
    scheduler_backend_->callDeferred();
  }

  void idontwant(MockPeer &peer, uint8_t i) {
    peer.write([&](pubsub::pb::RPC &rpc) {
      auto *idontwant = rpc.mutable_control()->add_idontwant();
      *idontwant->add_message_ids() = qtils::byte2str(encodeMessageId(i));
    });
    scheduler_backend_->callDeferred();
  }

  void heartbeat() {
    scheduler_backend_->shift(config_.heartbeat_interval_msec);
  }
};

/**
 * notify peers when subscribing and unsubscribing.
 */
TEST_F(GossipMockTest, SubscribeUnsubscribe) {
  setup();
  auto peer1 = connect(PeerKind::Floodsub);

  auto sub = subscribe();

  sub.cancel();
  scheduler_backend_->callDeferred();
  peer1->expect({.subscriptions = {false}});
}

/**
 * publish to subscribed peers only.
 * don't publish until peers subscribe.
 * don't publish after peers unsubscribe.
 */
TEST_F(GossipMockTest, PublishToFloodsub) {
  setup();
  auto peer1 = connect(PeerKind::Floodsub);

  publish(1);
  peer1->expect({});

  subscribe(*peer1, true);
  publish(2);
  peer1->expect({.messages = {2}});

  subscribe(*peer1, false);
  publish(3);
  peer1->expect({});
}

/**
 * forwards message to floodsub peers except:
 *  - peer who is not subscribed
 *  - peer who sent the message
 *  - message author
 * don't forward same message more than once.
 */
TEST_F(GossipMockTest, ForwardToFloodsub) {
  setup();
  auto peer1 = connect(PeerKind::Floodsub);
  auto peer2 = connect(PeerKind::Floodsub);
  auto peer3 = connect(PeerKind::Floodsub);
  auto peer4 = connect(PeerKind::Floodsub);

  auto sub = subscribe();
  subscribe(*peer1, true);
  subscribe(*peer2, true);
  subscribe(*peer3, true);
  publish(*peer2, 1, peer1->peer_id_);
  peer3->expect({.messages = {1}});

  publish(*peer2, 1, peer1->peer_id_);
}

/**
 * publish to fanout peers.
 * must publish to same initially chosen fanout peers.
 * don't forward messages to fanout peers.
 */
TEST_F(GossipMockTest, PublishToFanout) {
  setup();
  auto peer1 = connect(PeerKind::Gossipsub);
  auto peer2 = connect(PeerKind::Gossipsub);
  uint8_t i = 1;

  subscribe(*peer1, true);
  subscribe(*peer2, true);
  publish(i);
  if (peer1->received_.messages.empty()) {
    std::swap(peer1, peer2);
  }
  peer1->expect({.messages = {i}});
  i++;
  while (i < 30) {
    publish(i);
    peer1->expect({.messages = {i}});
    i++;
  }

  publish(*peer2, i);
}

/**
 * notify peers when grafting.
 * publish to grafted peers.
 * forward to grafted peers.
 */
TEST_F(GossipMockTest, PublishToMesh) {
  setup();
  auto peer1 = connect(PeerKind::Gossipsub);
  auto peer2 = connect(PeerKind::Gossipsub);

  auto sub = subscribe();
  subscribe(*peer1, true);
  subscribe(*peer2, true);
  peer1->expect({.graft = {true}});

  publish(1);
  peer1->expect({.messages = {1}});

  publish(*peer2, 2, randomPeerId());
  peer1->expect({.messages = {2}});
}

/**
 * publish to all peers.
 * don't forward to all peers.
 */
TEST_F(GossipMockTest, FloodPublish) {
  config_.flood_publish = true;
  setup();
  auto peer1 = connect(PeerKind::Gossipsub);
  auto peer2 = connect(PeerKind::Gossipsub);

  auto sub = subscribe();
  subscribe(*peer1, true);
  subscribe(*peer2, true);
  publish(1);
  publish(*peer2, 2, randomPeerId());
  peer1->expect({.messages = {1, 2}, .graft = {true}});
  peer2->expect({.messages = {1}});
}

/**
 * gossip recent messages to random peers.
 * don't gossip to mesh or fanout peers.
 */
TEST_F(GossipMockTest, Gossip) {
  setup();
  auto peer1 = connect(PeerKind::Gossipsub);
  auto peer2 = connect(PeerKind::Gossipsub);
  auto peer3 = connect(PeerKind::Gossipsub);

  auto sub = subscribe();
  subscribe(*peer1, true);
  subscribe(*peer2, true);
  subscribe(*peer3, true);
  publish(1);
  peer1->expect({.messages = {1}, .graft = {true}});

  heartbeat();
  if (peer2->received_.ihave.empty()) {
    std::swap(peer2, peer3);
  }
  peer2->expect({.ihave = {1}});
}

/**
 * send iwant after receiving ihave.
 * don't send iwant until subscribed to topic.
 * don't send iwant after receiving message.
 */
TEST_F(GossipMockTest, IhaveIwant) {
  setup();
  auto peer1 = connect(PeerKind::Gossipsub);

  ihave(*peer1, 1);
  peer1->expect({});

  auto sub = subscribe();
  ihave(*peer1, 1);
  peer1->expect({.iwant = {1}});

  publish(*peer1, 1);
  ihave(*peer1, 1);

  iwant(*peer1, 1);
  peer1->expect({.messages = {1}});
}

/**
 * send idontwant after receiving message.
 * send idontwant to mesh peers.
 * send idontwant to cancel pending iwant requests.
 * don't reply to iwant from peer after receiving idontwant.
 * don't forward message to peer after receiving idontwant.
 */
TEST_F(GossipMockTest, Idontwant) {
  config_.idontwant_message_size_threshold = 1;
  setup();
  auto peer1 = connect(PeerKind::Gossipsubv1_2);
  auto peer2 = connect(PeerKind::Gossipsubv1_2);

  auto sub = subscribe();
  subscribe(*peer1, true);
  subscribe(*peer2, true);
  publish(*peer2, 1);
  peer1->expect({.messages = {1}, .graft = {1}, .idontwant = {1}});

  ihave(*peer2, 2);
  peer2->expect({.iwant = {2}});
  publish(*peer1, 2);
  peer1->expect({.idontwant = {2}});
  peer2->expect({.idontwant = {2}});

  idontwant(*peer2, 2);
  iwant(*peer2, 2);
  peer2->expect({});

  idontwant(*peer2, 3);
  publish(*peer1, 3);
  peer1->expect({.idontwant = {3}});
  peer2->expect({});
}
