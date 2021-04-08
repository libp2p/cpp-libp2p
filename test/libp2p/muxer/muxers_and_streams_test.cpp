/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdlib>
#include <iosfwd>

#include <gtest/gtest.h>
#include <boost/di/extension/scopes/shared.hpp>

#include <libp2p/injector/host_injector.hpp>

#define TRACE_ENABLED 1
#include <libp2p/common/trace.hpp>

#include "testutil/prepare_loggers.hpp"

namespace libp2p::regression {

  enum WhatStream { ANY_STREAM, ACCEPTED_STREAM, CONNECTED_STREAM };

#define EVENTS(macro)                                                       \
  macro(NONE) macro(STARTED) macro(CONNECTED) macro(CONNECT_FAILURE)        \
      macro(ACCEPTED) macro(ACCEPT_FAILURE) macro(READ) macro(READ_FAILURE) \
          macro(WRITE) macro(WRITE_FAILURE) macro(FATAL_ERROR)

  struct Stats {
    enum Event {
#define DEF_EVENT(E) E,
      EVENTS(DEF_EVENT)
#undef DEF_EVENT
    };

    int node_id = 0;
    std::vector<Event> events;

    void put(Event e) {
      events.push_back(e);
    }

    Event lastEvent() const {
      if (events.empty()) {
        return NONE;
      }
      return events.back();
    }
  };

  std::ostream &operator<<(std::ostream &os, Stats::Event e) {
    switch (e) {
#define PRINT_EVENT(E) \
  case Stats::E:       \
    os << #E;          \
    break;
      EVENTS(PRINT_EVENT)
#undef PRINT_EVENT
      default:
        os << "<unknown>";
    }
    return os;
  }

  class Node : public std::enable_shared_from_this<Node> {
   public:
    using Behavior = std::function<void(Node &node)>;

    template <typename... InjectorArgs>
    Node(int node_id, bool jumbo_msg, const Behavior &behavior,
         std::shared_ptr<boost::asio::io_context> io, InjectorArgs &&...args)
        : behavior_(behavior) {
      stats_.node_id = node_id;
      auto injector =
          injector::makeHostInjector<boost::di::extension::shared_config>(
              boost::di::bind<boost::asio::io_context>.to(
                  io)[boost::di::override],

              std::forward<decltype(args)>(args)...);
      host_ = injector.template create<std::shared_ptr<Host>>();

      if (!jumbo_msg) {
        write_buf_ = std::make_shared<common::ByteArray>(getId().toVector());
      } else {
        static const size_t kJumboSize = 40 * 1024 * 1024;
        write_buf_ = std::make_shared<common::ByteArray>(kJumboSize, 0x99);
      }
      read_buf_ = std::make_shared<common::ByteArray>();
      read_buf_->resize(write_buf_->size());
    }

    const Stats &getStats() const {
      return stats_;
    }

    peer::PeerId getId() const {
      return host_->getId();
    }

    void connect(const peer::PeerInfo &connect_to) {
      start();
      // clang-format off
      host_->newStream(
          connect_to,
          getProtocolId(),
          [wptr = weak_from_this()] (auto rstream) {
            auto self = wptr.lock();
            if (self) {  self->onConnected(std::move(rstream)); }
          }
      );
      // clang-format on
    }

    void listen(const multi::Multiaddress &listen_to) {
      auto listen_res = host_->listen(listen_to);
      if (!listen_res) {
        TRACE("({}): Cannot listen to multiaddress {}, {}", stats_.node_id,
              listen_to.getStringAddress(), listen_res.error().message());
        stats_.put(Stats::FATAL_ERROR);
        return behavior_(*this);
      }
      start();
    }

    void read(WhatStream what_stream = ANY_STREAM) {
      auto stream = chooseStream(what_stream, true);
      if (!stream) {
        TRACE("({}): No stream to read from", stats_.node_id);
        stats_.put(Stats::FATAL_ERROR);
        return behavior_(*this);
      }
      // clang-format off
      stream->read(
          *read_buf_, read_buf_->size(),
          [wptr = weak_from_this(), buf = read_buf_] (auto res) {
            auto self = wptr.lock();
            if (self) {  self->onRead(res); }
           }
      );
      //clang-format on
    }

    void write(WhatStream what_stream = ANY_STREAM) {
      auto stream = chooseStream(what_stream, false);
      if (!stream) {
        TRACE("({}): No stream to write to", stats_.node_id);
        stats_.put(Stats::FATAL_ERROR);
        return behavior_(*this);
      }
      // clang-format off
      stream->write(
          *write_buf_, write_buf_->size(),
          [wptr = weak_from_this(), buf = write_buf_] (auto res) {
            auto self = wptr.lock();
            if (self) {  self->onWrite(res); }
          }
      );
      //clang-format on
    }

    void stop() {
      if (accepted_stream_) {
        auto p = accepted_stream_->remotePeerId();
        if (p) {
          host_->getNetwork().getConnectionManager()
          .closeConnectionsToPeer(p.value());
        }
      }
      if (connected_stream_) {
        auto p = connected_stream_->remotePeerId();
        if (p) {
          host_->getNetwork().getConnectionManager()
          .closeConnectionsToPeer(p.value());
        }
      }

      host_->stop();
    }

   private:
    const Behavior &behavior_;
    Stats stats_;
    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<connection::Stream> accepted_stream_;
    std::shared_ptr<connection::Stream> connected_stream_;
    std::shared_ptr<common::ByteArray> read_buf_;
    std::shared_ptr<common::ByteArray> write_buf_;
    bool started_ = false;

    void start() {
      if (!started_) {
        // clang-format off
        host_->setProtocolHandler(
            getProtocolId(),
            [wptr = weak_from_this()] (auto rstream) {
              auto self = wptr.lock();
              if (self) { self->onAccepted(std::move(rstream)); }
            }
        );
        // clang-format on

        host_->start();
        started_ = true;
        TRACE("({}): started", stats_.node_id);
        stats_.events.push_back(Stats::STARTED);
      }
    }

    static peer::Protocol getProtocolId() {
      return "/kocher/1.0.0";
    }

    void onAccepted(
        outcome::result<std::shared_ptr<connection::Stream>> rstream) {
      if (!rstream) {
        TRACE("({}): accept error: {}", stats_.node_id,
              rstream.error().message());
        stats_.put(Stats::ACCEPT_FAILURE);
      } else {
        stats_.put(Stats::ACCEPTED);
        accepted_stream_ = std::move(rstream.value());
      }
      behavior_(*this);
    }

    void onConnected(
        outcome::result<std::shared_ptr<connection::Stream>> rstream) {
      if (!rstream) {
        TRACE("({}): connect error: {}", stats_.node_id,
              rstream.error().message());
        stats_.put(Stats::CONNECT_FAILURE);
      } else {
        TRACE("({}): connected", stats_.node_id);
        stats_.put(Stats::CONNECTED);
        connected_stream_ = std::move(rstream.value());
      }
      behavior_(*this);
    }

    void onRead(outcome::result<size_t> res) {
      if (!res || res.value() != read_buf_->size()) {
        TRACE("({}): read error", stats_.node_id);
        stats_.put(Stats::READ_FAILURE);
      } else {
        TRACE("({}): read {} bytes", stats_.node_id, res.value());
        stats_.put(Stats::READ);
      }
      behavior_(*this);
    }

    void onWrite(outcome::result<size_t> res) {
      if (!res || res.value() != write_buf_->size()) {
        TRACE("({}): write error", stats_.node_id);
        stats_.put(Stats::WRITE_FAILURE);
      } else {
        TRACE("({}): written {} bytes", stats_.node_id, res.value());
        stats_.put(Stats::WRITE);
      }
      behavior_(*this);
    }

    std::shared_ptr<connection::Stream> chooseStream(WhatStream what_stream,
                                                     bool to_read) {
      if (what_stream == ACCEPTED_STREAM) {
        return accepted_stream_;
      }
      if (what_stream == CONNECTED_STREAM) {
        return connected_stream_;
      }
      if (to_read) {
        // prefer reads from inbound stream if it exists
        return accepted_stream_ ? accepted_stream_ : connected_stream_;
      }
      // prefer writes to outbound stream if it exists
      return connected_stream_ ? connected_stream_ : accepted_stream_;
    }
  };

  void runEventLoop(std::shared_ptr<boost::asio::io_context> io) {
    boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
    signals.async_wait(
        [&io](const boost::system::error_code &, int) { io->stop(); });

    auto max_duration = std::chrono::seconds(300);
    if (std::getenv("TRACE_DEBUG") != nullptr) {
      max_duration = std::chrono::seconds(86400);
    }

    io->run_for(max_duration);
  }

}  // namespace libp2p::regression

template <typename... InjectorArgs>
void testStreamsGetNotifiedAboutEOF(bool jumbo_msg, InjectorArgs &&...args) {
  using namespace libp2p::regression;  // NOLINT

  constexpr size_t kServerId = 0;
  constexpr size_t kClientId = 1;

  bool server_read = false;
  bool client_read = false;
  bool eof_passed = false;

  std::shared_ptr<boost::asio::io_context> io;
  std::shared_ptr<Node> client;
  std::shared_ptr<Node> server;

  Node::Behavior server_behavior = [&](Node &node) {
    auto stats = node.getStats();
    TRACE("Server event: {}", stats.lastEvent());
    switch (stats.lastEvent()) {
      case Stats::ACCEPTED:
      case Stats::WRITE:
        return node.read();
      case Stats::READ:
        server_read = true;
        return node.write();
      case Stats::READ_FAILURE:
      case Stats::WRITE_FAILURE:
        eof_passed = true;
        break;
      default:
        return;
    }
    io->stop();
  };

  Node::Behavior client_behavior = [&](Node &node) {
    auto stats = node.getStats();
    TRACE("Client event: {}", stats.lastEvent());
    switch (stats.lastEvent()) {
      case Stats::CONNECTED:
        return node.write();
      case Stats::WRITE:
        return node.read();
      case Stats::READ:
        TRACE("server eof");
        client_read = true;

        // disconnect
        node.stop();
        client.reset();

        return;
      default:
        break;
    }
    io->stop();
  };

  auto listen_to =
      libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40000").value();

  io = std::make_shared<boost::asio::io_context>();

  server = std::make_shared<Node>(kServerId, jumbo_msg, server_behavior, io,
                                  std::forward<decltype(args)>(args)...);
  client = std::make_shared<Node>(kClientId, jumbo_msg, client_behavior, io,
                                  std::forward<decltype(args)>(args)...);

  io->post([&]() {
    server->listen(listen_to);
    libp2p::peer::PeerInfo peer_info{server->getId(), {listen_to}};
    client->connect(peer_info);
  });

  runEventLoop(io);

  EXPECT_TRUE(server_read);
  EXPECT_TRUE(client_read);
  EXPECT_TRUE(eof_passed);

  if (server)
    server->stop();
  if (client)
    client->stop();
}

template <typename... InjectorArgs>
void testOutboundConnectionAcceptsStreams(InjectorArgs &&...args) {
  using namespace libp2p::regression;  // NOLINT

  constexpr size_t kServerId = 0;
  constexpr size_t kClientId = 1;

  bool client_accepted_stream = false;
  bool client_read_from_accepted_stream = false;
  bool server_read_from_connected_stream = false;

  std::shared_ptr<boost::asio::io_context> io;
  std::shared_ptr<Node> client;
  std::shared_ptr<Node> server;

  Node::Behavior server_behavior = [&](Node &node) {
    auto stats = node.getStats();
    TRACE("Server event: {}", stats.lastEvent());
    switch (stats.lastEvent()) {
      case Stats::ACCEPTED:
        // make reverse stream to peer, dont give any address in peerInfo
        // so dialer should reuse existing connection
        return node.connect(libp2p::peer::PeerInfo{client->getId(), {}});

      case Stats::CONNECTED:
        // write something through reverse stream
        return node.write(CONNECTED_STREAM);

      case Stats::WRITE:
        // read from reverse stream
        return node.read(CONNECTED_STREAM);

      case Stats::READ:
        server_read_from_connected_stream = true;
        break;

      default:
        break;
    }
    io->stop();
  };

  Node::Behavior client_behavior = [&](Node &node) {
    auto stats = node.getStats();
    TRACE("Client event: {}", stats.lastEvent());
    switch (stats.lastEvent()) {
      case Stats::CONNECTED:
        // do nothing, wait for inbound stream
        return;

      case Stats::ACCEPTED:
        client_accepted_stream = true;
        return node.read(ACCEPTED_STREAM);

      case Stats::READ:
        client_read_from_accepted_stream = true;
        return node.write(ACCEPTED_STREAM);

      case Stats::WRITE:
        return;

      default:
        break;
    }
    io->stop();
  };

  auto listen_to =
      libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40001").value();

  io = std::make_shared<boost::asio::io_context>();

  server = std::make_shared<Node>(kServerId, false, server_behavior, io,
                                  std::forward<decltype(args)>(args)...);
  client = std::make_shared<Node>(kClientId, false, client_behavior, io,
                                  std::forward<decltype(args)>(args)...);

  io->post([&]() {
    server->listen(listen_to);
    libp2p::peer::PeerInfo peer_info{server->getId(), {listen_to}};
    client->connect(peer_info);
  });

  runEventLoop(io);

  EXPECT_TRUE(client_accepted_stream);
  EXPECT_TRUE(client_read_from_accepted_stream);
  EXPECT_TRUE(server_read_from_connected_stream);

  if (server)
    server->stop();
  if (client)
    client->stop();
}

TEST(StreamsRegression, YamuxStreamsGetNotifiedAboutEOF) {
  testStreamsGetNotifiedAboutEOF(
      false,
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Yamux>()[boost::di::override]);
}

TEST(StreamsRegression, YamuxStreamsGetNotifiedAboutEOFJumboMsg) {
  testStreamsGetNotifiedAboutEOF(
      true,
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Yamux>()[boost::di::override]);
}


TEST(StreamsRegression, MplexStreamsGetNotifiedAboutEOF) {
  testStreamsGetNotifiedAboutEOF(
      false,
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Mplex>()[boost::di::override]);
}

TEST(StreamsRegression, OutboundMplexConnectionAcceptsStreams) {
  testOutboundConnectionAcceptsStreams(
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Mplex>()[boost::di::override]);
}

TEST(StreamsRegression, OutboundYamuxConnectionAcceptsStreams) {
  testOutboundConnectionAcceptsStreams(
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Yamux>()[boost::di::override]);
}

TEST(StreamsRegression, OutboundYamuxTLSConnectionAcceptsStreams) {
  testOutboundConnectionAcceptsStreams(
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Yamux>()[boost::di::override],
      libp2p::injector::useSecurityAdaptors<libp2p::security::TlsAdaptor>());
}

TEST(StreamsRegression, YamuxTLSStreamsGetNotifiedAboutEOF) {
  testStreamsGetNotifiedAboutEOF(
      false,
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Yamux>()[boost::di::override],
      libp2p::injector::useSecurityAdaptors<libp2p::security::TlsAdaptor>());
}

TEST(StreamsRegression, OutboundYamuxNoiseConnectionAcceptsStreams) {
  testOutboundConnectionAcceptsStreams(
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Yamux>()[boost::di::override],
      libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>());
}

TEST(StreamsRegression, YamuxNoiseStreamsGetNotifiedAboutEOF) {
  testStreamsGetNotifiedAboutEOF(
      false,
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Yamux>()[boost::di::override],
      libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>());
}

TEST(StreamsRegression, YamuxNoiseStreamsGetNotifiedAboutEOFJumboMsg) {
  testStreamsGetNotifiedAboutEOF(
      true,
      boost::di::bind<libp2p::muxer::MuxerAdaptor *[]>()
          .template to<libp2p::muxer::Yamux>()[boost::di::override],
      libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>());
}

int main(int argc, char *argv[]) {
  if (std::getenv("TRACE_DEBUG") != nullptr) {
    testutil::prepareLoggers(soralog::Level::TRACE);
  } else {
    testutil::prepareLoggers(soralog::Level::ERROR);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
