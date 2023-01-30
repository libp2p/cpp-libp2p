/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify/identify_msg_processor.hpp"

#include <tuple>

#include <generated/protocol/identify/protobuf/identify.pb.h>
#include <boost/assert.hpp>
#include <libp2p/basic/protobuf_message_read_writer.hpp>
#include <libp2p/network/network.hpp>
#include <libp2p/peer/address_repository.hpp>
#include <libp2p/protocol/identify/utils.hpp>

namespace {
  inline std::string fromMultiaddrToString(
      const libp2p::multi::Multiaddress &ma) {
    auto const &addr = ma.getBytesAddress();
    return {addr.begin(), addr.end()};
  }

  inline libp2p::outcome::result<libp2p::multi::Multiaddress>
  fromStringToMultiaddr(const std::string &addr) {
    return libp2p::multi::Multiaddress::create(gsl::span<const uint8_t>(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-narrowing-conversions)
        reinterpret_cast<const uint8_t *>(addr.data()), addr.size()));
  }
}  // namespace

namespace libp2p::protocol {
  IdentifyMessageProcessor::IdentifyMessageProcessor(
      Host &host, network::ConnectionManager &conn_manager,
      peer::IdentityManager &identity_manager,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : host_{host},
        conn_manager_{conn_manager},
        identity_manager_{identity_manager},
        key_marshaller_{std::move(key_marshaller)} {
    BOOST_ASSERT(key_marshaller_);
  }

  boost::signals2::connection IdentifyMessageProcessor::onIdentifyReceived(
      const std::function<IdentifyCallback> &cb) {
    return signal_identify_received_.connect(cb);
  }

  void IdentifyMessageProcessor::sendIdentify(StreamSPtr stream) {
    identify::pb::Identify msg;

    // set the protocols we speak on
    for (const auto &proto : host_.getRouter().getSupportedProtocols()) {
      msg.add_protocols(proto);
    }

    // set an address of the other side, so that it knows, which address we used
    // to connect to it
    if (auto remote_addr = stream->remoteMultiaddr()) {
      msg.set_observedaddr(fromMultiaddrToString(remote_addr.value()));
    }

    // set addresses we are available on
    for (const auto &addr : host_.getPeerInfo().addresses) {
      msg.add_listenaddrs(fromMultiaddrToString(addr));
    }

    // set our public key
    auto marshalled_pubkey_res =
        key_marshaller_->marshal(identity_manager_.getKeyPair().publicKey);
    if (!marshalled_pubkey_res) {
      log_->critical(
          "cannot marshal public key, which was provided to us by the identity "
          "manager: {}",
          marshalled_pubkey_res.error().message());
    } else {
      auto &&marshalled_pubkey = marshalled_pubkey_res.value();
      msg.set_publickey(marshalled_pubkey.key.data(),
                        marshalled_pubkey.key.size());
    }

    // set versions of Libp2p and our implementation
    msg.set_protocolversion(std::string{host_.getLibp2pVersion()});
    msg.set_agentversion(std::string{host_.getLibp2pClientVersion()});

    // write the resulting Protobuf message
    auto rw = std::make_shared<basic::ProtobufMessageReadWriter>(stream);
    rw->write<identify::pb::Identify>(
        msg,
        [self{shared_from_this()},
         stream = std::move(stream)](auto &&res) mutable {
          self->identifySent(std::forward<decltype(res)>(res), stream);
        });
  }

  void IdentifyMessageProcessor::identifySent(
      outcome::result<size_t> written_bytes, const StreamSPtr &stream) {
    auto [peer_id, peer_addr] = detail::getPeerIdentity(stream);
    if (!written_bytes) {
      log_->error("cannot write identify message to stream to peer {}, {}: {}",
                  peer_id, peer_addr, written_bytes.error().message());
      return stream->reset();
    }

    log_->info("successfully written an identify message to peer {}, {}",
               peer_id, peer_addr);

    stream->close([self{shared_from_this()}, p = std::move(peer_id),
                   a = std::move(peer_addr)](auto &&res) {
      if (!res) {
        self->log_->error("cannot close the stream to peer {}, {}: {}", p, a,
                          res.error().message());
      }
    });
  }

  void IdentifyMessageProcessor::receiveIdentify(StreamSPtr stream) {
    auto rw = std::make_shared<basic::ProtobufMessageReadWriter>(stream);
    rw->read<identify::pb::Identify>(
        [self{shared_from_this()}, s = std::move(stream)](auto &&res) {
          self->identifyReceived(std::forward<decltype(res)>(res), s);
        });
  }

  Host &IdentifyMessageProcessor::getHost() const noexcept {
    return host_;
  }

  network::ConnectionManager &IdentifyMessageProcessor::getConnectionManager()
      const noexcept {
    return conn_manager_;
  }

  const ObservedAddresses &IdentifyMessageProcessor::getObservedAddresses()
      const noexcept {
    return observed_addresses_;
  }

  void IdentifyMessageProcessor::identifyReceived(
      outcome::result<identify::pb::Identify> msg_res,
      const StreamSPtr &stream) {
    auto [peer_id_str, peer_addr_str] = detail::getPeerIdentity(stream);
    if (!msg_res) {
      log_->error("cannot read an identify message from peer {}, {}: {}",
                  peer_id_str, peer_addr_str, msg_res.error());
      return stream->reset();
    }

    log_->info("received an identify message from peer {}, {}", peer_id_str,
               peer_addr_str);
    stream->close([self{shared_from_this()}, p = std::move(peer_id_str),
                   a = std::move(peer_addr_str)](auto &&res) {
      if (!res) {
        self->log_->error("cannot close the stream to peer {}, {}: {}", p, a,
                          res.error().message());
      }
    });

    auto &&msg = std::move(msg_res.value());

    // process a received public key and retrieve an ID of the other peer
    auto received_pubkey_str = msg.has_publickey() ? msg.publickey() : "";
    auto peer_id_opt = consumePublicKey(stream, received_pubkey_str);
    if (!peer_id_opt) {
      // something bad happened during key processing - we can't continue
      return;
    }
    auto peer_id = std::move(*peer_id_opt);

    // store the received protocols
    std::vector<peer::ProtocolName> protocols;
    for (const auto &proto : msg.protocols()) {
      protocols.push_back(proto);
    }
    auto add_res =
        host_.getPeerRepository().getProtocolRepository().addProtocols(
            peer_id, protocols);
    if (!add_res) {
      log_->error("cannot add protocols to peer {}: {}", peer_id.toBase58(),
                  add_res.error().message());
    }

    if (msg.has_observedaddr()) {
      consumeObservedAddresses(msg.observedaddr(), peer_id, stream);
    }

    std::vector<std::string> addresses;
    for (const auto &addr : msg.listenaddrs()) {
      addresses.push_back(addr);
    }
    consumeListenAddresses(addresses, peer_id);

    signal_identify_received_(peer_id);
  }

  boost::optional<peer::PeerId> IdentifyMessageProcessor::consumePublicKey(
      const StreamSPtr &stream, std::string_view pubkey_str) {
    auto stream_peer_id_res = stream->remotePeerId();

    // if we haven't received a key from the other peer, all we can do is to
    // return the already known peer id
    if (pubkey_str.empty()) {
      if (!stream_peer_id_res) {
        return boost::none;
      }
      return stream_peer_id_res.value();
    }

    // peer id can be set in stream, derived from the received public key or
    // both; handle all possible cases
    boost::optional<peer::PeerId> stream_peer_id;
    boost::optional<crypto::PublicKey> pubkey;

    // retrieve a peer id from the stream
    if (stream_peer_id_res) {
      stream_peer_id = std::move(stream_peer_id_res.value());
    }

    // unmarshal a received public key
    std::vector<uint8_t> pubkey_buf;
    pubkey_buf.insert(pubkey_buf.end(), pubkey_str.begin(), pubkey_str.end());
    auto pubkey_res =
        key_marshaller_->unmarshalPublicKey(crypto::ProtobufKey{pubkey_buf});
    if (!pubkey_res) {
      log_->info("cannot unmarshal public key for peer {}: {}",
                 stream_peer_id ? stream_peer_id->toBase58() : "",
                 pubkey_res.error().message());
      return stream_peer_id;
    }
    pubkey = std::move(pubkey_res.value());

    // derive a peer id from the received public key; PeerId is made from
    // Protobuf-marshalled key, so we use it here
    auto msg_peer_id_res =
        peer::PeerId::fromPublicKey(crypto::ProtobufKey{pubkey_buf});
    if (!msg_peer_id_res) {
      log_->info("cannot derive PeerId from the received key: {}",
                 msg_peer_id_res.error().message());
      return stream_peer_id;
    }
    auto msg_peer_id = std::move(msg_peer_id_res.value());

    auto &key_repo = host_.getPeerRepository().getKeyRepository();
    if (!stream_peer_id) {
      // didn't know the ID before; memorize the key, from which it can be
      // derived later
      auto add_res = key_repo.addPublicKey(msg_peer_id, *pubkey);
      if (!add_res) {
        log_->error("cannot add key to the repo of peer {}: {}",
                    msg_peer_id.toBase58(), add_res.error().message());
      }
      return msg_peer_id;
    }

    if (stream_peer_id && *stream_peer_id != msg_peer_id) {
      log_->error(
          "peer with id {} sent public key, which derives to id {}, but they "
          "must be equal",
          stream_peer_id->toBase58(), msg_peer_id.toBase58());
      return boost::none;
    }

    // insert the derived key into key repository
    auto add_res = key_repo.addPublicKey(*stream_peer_id, *pubkey);
    if (!add_res) {
      log_->error("cannot add key to the repo of peer {}: {}",
                  stream_peer_id->toBase58(), add_res.error().message());
    }
    return stream_peer_id;
  }

  void IdentifyMessageProcessor::consumeObservedAddresses(
      const std::string &address_str, const peer::PeerId &peer_id,
      const StreamSPtr &stream) {
    // in order for observed addresses feature to work, all those parameters
    // must be gotten
    auto remote_addr_res = stream->remoteMultiaddr();
    auto local_addr_res = stream->localMultiaddr();
    auto is_initiator_res = stream->isInitiator();
    if (!remote_addr_res || !local_addr_res || !is_initiator_res) {
      return;
    }

    auto address_res = fromStringToMultiaddr(address_str);
    if (!address_res) {
      return log_->error("peer {} has send an invalid observed address",
                         peer_id.toBase58());
    }
    auto &&observed_address = address_res.value();

    // if our local address is not one of our "official" listen addresses, we
    // are not going to save its mapping to the observed one
    auto &listener = host_.getNetwork().getListener();
    auto i_listen_addresses = listener.getListenAddressesInterfaces();

    auto listen_addresses = listener.getListenAddresses();

    auto addr_in_addresses =
        std::find(i_listen_addresses.begin(), i_listen_addresses.end(),
                  local_addr_res.value())
            != i_listen_addresses.end()
        || std::find(listen_addresses.begin(), listen_addresses.end(),
                     local_addr_res.value())
            != listen_addresses.end();
    if (!addr_in_addresses) {
      return;
    }

    if (!hasConsistentTransport(observed_address, host_.getAddresses())) {
      return;
    }

    observed_addresses_.add(std::move(observed_address),
                            std::move(local_addr_res.value()),
                            remote_addr_res.value(), is_initiator_res.value());
  }

  bool IdentifyMessageProcessor::hasConsistentTransport(
      const multi::Multiaddress &ma, gsl::span<const multi::Multiaddress> mas) {
    auto ma_protos = ma.getProtocols();
    return std::any_of(mas.begin(), mas.end(),
                       [&ma_protos](const auto &ma_from_mas) {
                         return ma_protos == ma_from_mas.getProtocols();
                       });
  }

  void IdentifyMessageProcessor::consumeListenAddresses(
      gsl::span<const std::string> addresses_strings,
      const peer::PeerId &peer_id) {
    if (addresses_strings.empty()) {
      return;
    }

    std::vector<multi::Multiaddress> listen_addresses;
    for (const auto &addr_str : addresses_strings) {
      auto addr_res = fromStringToMultiaddr(addr_str);
      if (!addr_res) {
        log_->error("peer {} has sent an invalid listen address",
                    peer_id.toBase58());
        continue;
      }
      listen_addresses.push_back(std::move(addr_res.value()));
    }

    auto &addr_repo = host_.getPeerRepository().getAddressRepository();

    // invalidate previously known addresses of that peer
    auto update_res = addr_repo.updateAddresses(peer_id, peer::ttl::kTransient);
    if (!update_res) {
      SL_DEBUG(log_, "cannot update listen addresses of the peer {}: {}",
               peer_id.toBase58(), update_res.error().message());
    }

    // memorize the addresses
    auto addresses = addr_repo.getAddresses(peer_id);
    if (!addresses) {
      SL_DEBUG(log_, "can not get addresses for peer {}", peer_id.toBase58());
    }

    bool permanent_ttl =
        (addresses
         && (conn_manager_.getBestConnectionForPeer(peer_id) != nullptr));

    auto upsert_res = addr_repo.upsertAddresses(
        peer_id, listen_addresses,
        permanent_ttl ? peer::ttl::kPermanent : peer::ttl::kRecentlyConnected);

    if (!upsert_res) {
      log_->error("cannot add addresses to peer {}: {}", peer_id.toBase58(),
                  upsert_res.error().message());
    }
  }
}  // namespace libp2p::protocol
