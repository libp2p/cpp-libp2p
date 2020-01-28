/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <spdlog/fmt/fmt.h>
#include <boost/program_options.hpp>

#include <libp2p/injector/gossip_injector.hpp>

#include "console_async_reader.hpp"
#include "utility.hpp"

namespace {
  // cmd line options
  struct Options {
    // local node port
    int port = 0;

    // topic name
    std::string topic = "chat";

    // optional remote peer to connect to
    boost::optional<libp2p::peer::PeerInfo> remote;

    // log level: 'd' for debug, 'i' for info, 'w' for warning, 'e' for error
    char log_level = 'w';
  };

  // parses command line, returns non-empty Options on success
  boost::optional<Options> parseCommandLine(int argc, char **argv);
}  // namespace

int main(int argc, char *argv[]) {
  namespace utility = libp2p::protocol::example::utility;

  auto options = parseCommandLine(argc, argv);
  if (!options) {
    return 1;
  }

  utility::setupLoggers(options->log_level);

  // overriding default config to see local messages as well (echo mode)
  libp2p::protocol::gossip::Config config;
  config.echo_forward_mode = true;

  // injector creates and ties dependent objects
  auto injector = libp2p::injector::makeGossipInjector(
      libp2p::injector::useGossipConfig(config));

  // create asio context
  auto io = injector.create<std::shared_ptr<boost::asio::io_context>>();

  // host is our local libp2p node
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  // make peer uri of local node
  auto local_address_str =
      fmt::format("/ip4/{}/tcp/{}/p2p/{}", utility::getLocalIP(*io),
                  options->port, host->getId().toBase58());

  // local address -> peer info
  auto peer_info = utility::str2peerInfo(local_address_str);
  if (!peer_info) {
    std::cerr << "Cannot resolve local peer from " << local_address_str << "\n";
    return 2;
  }

  // say local address
  std::cerr << "I am " << local_address_str << "\n";

  // create gossip node
  auto gossip =
      injector.create<std::shared_ptr<libp2p::protocol::gossip::Gossip>>();

  using Message = libp2p::protocol::gossip::Gossip::Message;

  // subscribe to chat topic, print messages to the console
  auto subscription = gossip->subscribe(
      {options->topic}, [](boost::optional<const Message &> m) {
        if (!m) {
          // message with no value means EOS, this occurs when the node has
          // stopped
          return;
        }
        std::cerr << utility::formatPeerId(m->from) << ": "
                  << utility::toString(m->data) << "\n";
      });

  // tell gossip to connect to remote peer, only if specified
  if (options->remote) {
    gossip->addBootstrapPeer(options->remote->id,
                             options->remote->addresses[0]);
  }

  // start the node as soon as async engine starts
  io->post([&] {
    auto listen_res = host->listen(peer_info->addresses[0]);
    if (!listen_res) {
      std::cerr << "Cannot listen to multiaddress "
                << peer_info->addresses[0].getStringAddress() << ", "
                << listen_res.error().message() << "\n";
      io->stop();
      return;
    }
    host->start();
    gossip->start();
    std::cerr << "Node started\n";
  });

  // read lines from stdin in async manner and publish them into the chat
  utility::ConsoleAsyncReader stdin_reader(
      *io, [&gossip, &options](const std::string &msg) {
        gossip->publish({options->topic}, utility::fromString(msg));
      });

  // gracefully shutdown on signal
  boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
  signals.async_wait(
      [&io](const boost::system::error_code &, int) { io->stop(); });

  // run event loop
  io->run();
  std::cerr << "Node stopped\n";

  return 0;
}

namespace {

  boost::optional<Options> parseCommandLine(int argc, char **argv) {
    namespace po = boost::program_options;
    try {
      Options o;
      std::string remote;

      po::options_description desc("gossip_chat_example options");
      desc.add_options()("help,h", "print usage message")(
          "port,p", po::value(&o.port), "port to listen to")(
          "topic,t", po::value(&o.topic), "chat topic name (default is 'chat'")(
          "remote,r", po::value(&remote), "remote peer uri to connect to")(
          "log,l", po::value(&o.log_level), "log level, [e,w,i,d]");

      po::variables_map vm;
      po::store(parse_command_line(argc, argv, desc), vm);
      po::notify(vm);

      if (vm.count("help") != 0 || argc == 1) {
        std::cerr << desc << "\n";
        return boost::none;
      }

      if (o.port == 0) {
        std::cerr << "Port cannot be zero\n";
        return boost::none;
      }

      if (o.topic.empty()) {
        std::cerr << "Topic name cannot be empty\n";
        return boost::none;
      }

      if (!remote.empty()) {
        o.remote = libp2p::protocol::example::utility::str2peerInfo(remote);
        if (!o.remote) {
          std::cerr << "Cannot resolve remote peer address from " << remote
                    << "\n";
          return boost::none;
        }
      }

      return o;

    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
    }
    return boost::none;
  }

}  // namespace
