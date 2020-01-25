/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include <spdlog/fmt/fmt.h>

#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include "factory.hpp"
#include "utility.hpp"

namespace {

  /// local logger
  libp2p::common::Logger logger;

}  // namespace

namespace libp2p::protocol::gossip::example {

  // Logic of this host
  class HostContext {
    // publishes this peer beacon into topic "peers" once a minute
    static const size_t kAnnounceInterval = 60000;

    // this libp2p host
    std::shared_ptr<Host> host_;

    // local pubsub node
    std::shared_ptr<Gossip> gossip_;

    // local endpoint info to setup this peer id and listen address
    EndpointInfo local_ep_;

    // self listening address URI, sent into "peers" topic
    std::string self_address_str_;

    // subscription handle to "peers" topic
    Subscription peers_sub_;

    // subscription handler to "chat" topic
    Subscription chat_sub_;

    // timer handle to perform periodic self-announces
    Scheduler::Handle self_announce_timer_;

    // callback from "peers" topic
    void onPeerAnnounce(Gossip::SubscriptionData d) {
      if (d) {
        std::string s = toString(d->data);
        logger->info("Peer announced: {}", s);
        auto pi = str2peerInfo(s);
        if (pi) {
          // add bootstrap peer into gossip node
          gossip_->addBootstrapPeer(pi.value().id, pi.value().addresses[0]);
        }
      }
    }

    // callback from "chat" topic, contains message
    static void onChatMessage(Gossip::SubscriptionData d) {
      if (d) {
        fmt::print(stderr, "{}: {}\n", formatPeerId(d->from),
                   toString(d->data));
      }
    }

    // timer callback, publishes self address tino "peers"
    void onSelfAnnounce() {
      publish("peers", self_address_str_);
      self_announce_timer_.reschedule(kAnnounceInterval);
    }

    // called when asio starts
    void onStart() {
      auto listen_res = host_->listen(local_ep_.address.value());
      if (!listen_res) {
        fmt::print(stderr, "Cannot listen to multiaddress {}, {}",
                   local_ep_.address.value().getStringAddress(),
                   listen_res.error().message());
        return;
      }

      host_->start();
      gossip_->start();
      publish("peers", self_address_str_);
      fmt::print(stderr, "Started\n");
    }

   public:
    HostContext(const std::shared_ptr<Scheduler> &scheduler,
                const std::shared_ptr<boost::asio::io_context> &io,
                EndpointInfo local_endpoint,
                const boost::optional<EndpointInfo> &remote_endpoint)
        : local_ep_(std::move(local_endpoint)) {
      self_address_str_ =
          fmt::format("{}/ipfs/{}", local_ep_.address->getStringAddress(),
                      local_ep_.peer_id.toBase58());
      Config config;
      config.echo_forward_mode = true;

      // create host and gossip nodes using our keypair
      std::tie(host_, gossip_) = createHostAndGossip(
          config, scheduler, io, std::move(local_ep_.keypair));

      // subscribe to "peers"
      peers_sub_ = gossip_->subscribe(
          {"peers"}, [this](Gossip::SubscriptionData d) { onPeerAnnounce(d); });

      // subscribe to "chat"
      chat_sub_ = gossip_->subscribe(
          {"chat"}, [](Gossip::SubscriptionData d) { onChatMessage(d); });

      // subscribe to announce timer
      self_announce_timer_ = scheduler->schedule(
          kAnnounceInterval, [this]() { onSelfAnnounce(); });

      // add remote peer to dial to
      if (remote_endpoint) {
        gossip_->addBootstrapPeer(remote_endpoint->peer_id,
                                  remote_endpoint->address);
      }

      // defer onStart() call to asio startup
      scheduler->schedule([this] { onStart(); }).detach();
    }

    // publishes string message to topic
    void publish(const TopicId &topic, const std::string &msg) {
      gossip_->publish({topic}, fromString(msg));
    }
  };

  // helper with console print
  auto makeEndpoint(const std::string_view &label, const std::string &host,
                    const std::string &port, const std::string &key) {
    auto ep = makeEndpoint(host, port, key);
    if (!ep) {
      fmt::print(stderr,
                 "Cannot make {} endpoint out of HOST={}, PORT={} and KEY={}\n",
                 label, host, port, key);
    } else {
      fmt::print(stderr, "{} peer id: {}, listen address: {}\n", label,
                 ep.value().peer_id.toBase58(),
                 ep.value().address.value().getStringAddress());
    }
    return ep;
  }

  // asynchronously reads lines from stdin
  class ConsoleAsyncReader {
   public:
    // lines read from the console come into this callback
    using Handler = std::function<void(const std::string &)>;

    // starts the reader
    ConsoleAsyncReader(boost::asio::io_context &io, Handler handler)
        : in_(io, STDIN_FILENO), handler_(std::move(handler)) {
      read();
    }

    void stop() {
      stopped_ = true;
    }

   private:
    void read() {
      input_.consume(input_.data().size());
      boost::asio::async_read_until(
          in_, input_, "\n",
          [this](const boost::system::error_code &e, std::size_t size) {
            onRead(e, size);
          });
    }

    void onRead(const boost::system::error_code &e, std::size_t size) {
      if (stopped_) {
        return;
      }
      if (!e && size != 0) {
        line_.assign(buffers_begin(input_.data()), buffers_end(input_.data()));
        line_.erase(line_.find_first_of("\r\n"));
        handler_(line_);
      } else {
        logger->error(e.message());
      }
      read();
    };

    boost::asio::posix::stream_descriptor in_;
    boost::asio::streambuf input_;
    std::string line_;
    Handler handler_;
    bool stopped_ = false;
  };

}  // namespace libp2p::protocol::gossip::example

int main(int argc, char *argv[]) {
  namespace example = libp2p::protocol::gossip::example;

  if (argc != 6 && argc != 3) {
    fmt::print(
        stderr,
        "Usage:\n LOCAL_PORT LOCAL_KEY [REMOTE_HOST REMOTE_PORT REMOTE_KEY]\n");
    return 1;
  }

  auto io = std::make_shared<boost::asio::io_context>();

  auto local_endpoint = example::makeEndpoint("local", example::getLocalIP(*io),
                                              argv[1], argv[2]);  // NOLINT
  if (!local_endpoint) {
    return 2;
  }

  boost::optional<example::EndpointInfo> remote_endpoint;
  if (argc == 6) {
    remote_endpoint =
        example::makeEndpoint("remote", argv[3], argv[4], argv[5]);  // NOLINT
    if (!remote_endpoint) {
      return 3;
    }
  }

  logger = example::setupLoggers(true);

  auto scheduler = std::make_shared<libp2p::protocol::AsioScheduler>(*io, 100);

  example::HostContext host(scheduler, io, std::move(local_endpoint.value()),
                            remote_endpoint);

  logger->info("Starting");

  // reads messages from console and publishes them
  example::ConsoleAsyncReader stdin_reader(
      *io, [&host](const std::string &msg) { host.publish("chat", msg); });

  // run until signal is caught
  boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
  signals.async_wait(
      [&io](const boost::system::error_code &, int) { io->stop(); });
  io->run();

  fmt::print(stderr, "Stopped\n");
  return 0;
}
