/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_EXAMPLE_FACTORY_HPP
#define LIBP2P_KAD_EXAMPLE_FACTORY_HPP

#include <libp2p/host/host.hpp>
#include <libp2p/crypto/key_generator.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <memory>

namespace libp2p::kad_example {
  std::shared_ptr<boost::asio::io_context> createIOContext();

  struct PerHostObjects {
    std::shared_ptr<libp2p::Host> host;
    std::shared_ptr<libp2p::protocol::kademlia::RoutingTable> routing_table;
    std::shared_ptr<libp2p::crypto::KeyGenerator> key_gen;
    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller;
  };

  void createPerHostObjects(PerHostObjects& objects);

  std::optional<libp2p::peer::PeerInfo> str2peerInfo(const std::string& str);

} //namespace

#endif //LIBP2P_KAD_EXAMPLE_FACTORY_HPP
