/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/impl/upgrader_impl.hpp>

#include <fmt/format.h>
#include <libp2p/multi/converters/conversion_error.hpp>
#include <numeric>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::transport, UpgraderImpl::Error, e) {
  using E = libp2p::transport::UpgraderImpl::Error;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::NO_ADAPTOR_FOUND:
      return "can not find suitable adaptor";
  }
  return "unknown error";
}

namespace {
  template <typename AdaptorType>
  std::shared_ptr<AdaptorType> findAdaptor(
      const std::vector<std::shared_ptr<AdaptorType>> &adaptors,
      const libp2p::peer::ProtocolName &proto) {
    auto adaptor = std::find_if(
        adaptors.begin(), adaptors.end(),
        [&proto](const auto &a) { return proto == a->getProtocolId(); });

    if (adaptor != adaptors.end()) {
      return *adaptor;
    }
    return nullptr;
  }
}  // namespace

namespace libp2p::transport {
  UpgraderImpl::UpgraderImpl(
      std::shared_ptr<protocol_muxer::ProtocolMuxer> protocol_muxer,
      std::vector<LayerAdaptorSPtr> layer_adaptors,
      std::vector<SecAdaptorSPtr> security_adaptors,
      std::vector<MuxAdaptorSPtr> muxer_adaptors)
      : protocol_muxer_{std::move(protocol_muxer)},
        layer_adaptors_{std::move(layer_adaptors)},
        security_adaptors_{std::move(security_adaptors)},
        muxer_adaptors_{std::move(muxer_adaptors)} {
    BOOST_ASSERT(protocol_muxer_ != nullptr);

    BOOST_ASSERT(std::all_of(layer_adaptors_.begin(), layer_adaptors_.end(),
                             [](auto &&ptr) { return ptr != nullptr; }));

    BOOST_ASSERT_MSG(!security_adaptors_.empty(),
                     "upgrader has no security adaptors");
    BOOST_ASSERT(std::all_of(security_adaptors_.begin(),
                             security_adaptors_.end(),
                             [](auto &&ptr) { return ptr != nullptr; }));

    BOOST_ASSERT_MSG(!muxer_adaptors_.empty(),
                     "upgrader got no muxer adaptors");
    BOOST_ASSERT(std::all_of(muxer_adaptors_.begin(), muxer_adaptors_.end(),
                             [](auto &&ptr) { return ptr != nullptr; }));

    // so that we don't need to extract lists of supported protocols every time
    std::transform(
        security_adaptors_.begin(), security_adaptors_.end(),
        std::back_inserter(security_protocols_),
        [](const auto &adaptor) { return adaptor->getProtocolId(); });

    std::transform(
        muxer_adaptors_.begin(), muxer_adaptors_.end(),
        std::back_inserter(muxer_protocols_),
        [](const auto &adaptor) { return adaptor->getProtocolId(); });
  }

  void UpgraderImpl::upgradeLayersInbound(RawSPtr conn, ProtoAddrVec layers,
                                          OnLayerCallbackFunc cb) {
    upgradeToNextLayerInbound(std::move(conn), std::move(layers), 0,
                              std::move(cb));
  }

  void UpgraderImpl::upgradeLayersOutbound(const multi::Multiaddress &address,
                                           RawSPtr conn, ProtoAddrVec layers,
                                           OnLayerCallbackFunc cb) {
    upgradeToNextLayerOutbound(address, std::move(conn), std::move(layers), 0,
                               std::move(cb));
  }

  void UpgraderImpl::upgradeToNextLayerInbound(LayerSPtr conn,
                                               ProtoAddrVec layers,
                                               size_t layer_index,
                                               OnLayerCallbackFunc cb) {
    BOOST_ASSERT_MSG(!conn->isInitiator(),
                     "connection is initiator, and upgrade for inbound is "
                     "called (should be upgrade for outbound)");

    if (layer_index >= layers.size()) {
      return cb(conn);
    }
    const auto &protocol = layers[layer_index];

    auto adaptor_it =
        std::find_if(layer_adaptors_.begin(), layer_adaptors_.end(),
                     [&](const auto &adaptor) {
                       return adaptor->getProtocol() == protocol.first.code;
                     });

    if (adaptor_it == layer_adaptors_.end()) {
      return cb(multi::converters::ConversionError::NOT_IMPLEMENTED);
    }

    const auto &adaptor = *adaptor_it;

    return adaptor->upgradeInbound(
        conn,
        [self{shared_from_this()}, layers = std::move(layers), layer_index,
         cb{std::move(cb)}](
            outcome::result<LayerSPtr> next_layer_conn_res) mutable {
          if (next_layer_conn_res.has_error()) {
            return cb(next_layer_conn_res.as_failure());
          }
          auto next_layer_conn = std::move(next_layer_conn_res.value());

          self->upgradeToNextLayerInbound(std::move(next_layer_conn),
                                          std::move(layers), layer_index + 1,
                                          std::move(cb));
        });
  }

  void UpgraderImpl::upgradeToNextLayerOutbound(
      const multi::Multiaddress &address, LayerSPtr conn, ProtoAddrVec layers,
      size_t layer_index, OnLayerCallbackFunc cb) {
    BOOST_ASSERT_MSG(conn->isInitiator(),
                     "connection is NOT initiator, and upgrade of outbound is "
                     "called (should be upgrade of inbound)");

    if (layer_index >= layers.size()) {
      return cb(conn);
    }
    const auto &protocol = layers[layer_index];

    auto adaptor_it =
        std::find_if(layer_adaptors_.begin(), layer_adaptors_.end(),
                     [&](const auto &adaptor) {
                       return adaptor->getProtocol() == protocol.first.code;
                     });

    if (adaptor_it == layer_adaptors_.end()) {
      return cb(multi::converters::ConversionError::NOT_IMPLEMENTED);
    }

    const auto &adaptor = *adaptor_it;

    return adaptor->upgradeOutbound(
        address, conn,
        [=, self{shared_from_this()}, layers = std::move(layers),
         cb{std::move(cb)}](
            outcome::result<LayerSPtr> next_layer_conn_res) mutable {
          if (next_layer_conn_res.has_error()) {
            return cb(next_layer_conn_res.error());
          }
          auto &next_layer_conn = next_layer_conn_res.value();
          self->upgradeToNextLayerOutbound(address, std::move(next_layer_conn),
                                           std::move(layers), layer_index + 1,
                                           std::move(cb));
        });
  }

  void UpgraderImpl::upgradeToSecureInbound(LayerSPtr conn,
                                            OnSecuredCallbackFunc cb) {
    BOOST_ASSERT_MSG(!conn->isInitiator(),
                     "connection is initiator, and upgrade for inbound is "
                     "called (should be upgrade for outbound)");

    protocol_muxer_->selectOneOf(
        security_protocols_, conn, conn->isInitiator(), true,
        [self{shared_from_this()}, cb = std::move(cb),
         conn](outcome::result<peer::ProtocolName> proto_res) mutable {
          if (!proto_res) {
            return cb(proto_res.error());
          }

          auto adaptor =
              findAdaptor(self->security_adaptors_, proto_res.value());
          if (adaptor == nullptr) {
            return cb(Error::NO_ADAPTOR_FOUND);
          }

          return adaptor->secureInbound(std::move(conn), std::move(cb));
        });
  }

  void UpgraderImpl::upgradeToSecureOutbound(LayerSPtr conn,
                                             const peer::PeerId &remoteId,
                                             OnSecuredCallbackFunc cb) {
    BOOST_ASSERT_MSG(conn->isInitiator(),
                     "connection is NOT initiator, and upgrade for outbound is "
                     "called (should be upgrade for inbound)");

    protocol_muxer_->selectOneOf(
        security_protocols_, conn, conn->isInitiator(), true,
        [self{shared_from_this()}, cb = std::move(cb), conn,
         remoteId](outcome::result<peer::ProtocolName> proto_res) mutable {
          if (!proto_res) {
            return cb(proto_res.error());
          }

          auto adaptor =
              findAdaptor(self->security_adaptors_, proto_res.value());
          if (adaptor == nullptr) {
            return cb(Error::NO_ADAPTOR_FOUND);
          }

          return adaptor->secureOutbound(std::move(conn), remoteId,
                                         std::move(cb));
        });
  }

  void UpgraderImpl::upgradeToMuxed(SecSPtr conn, OnMuxedCallbackFunc cb) {
    return protocol_muxer_->selectOneOf(
        muxer_protocols_, conn, conn->isInitiator(), true,
        [self{shared_from_this()}, cb = std::move(cb),
         conn](outcome::result<peer::ProtocolName> proto_res) mutable {
          if (!proto_res) {
            return cb(proto_res.error());
          }

          auto adaptor = findAdaptor(self->muxer_adaptors_, proto_res.value());
          if (!adaptor) {
            return cb(Error::NO_ADAPTOR_FOUND);
          }

          return adaptor->muxConnection(
              std::move(conn),
              [cb = std::move(cb)](outcome::result<CapSPtr> conn_res) {
                if (!conn_res) {
                  return cb(conn_res.error());
                }

                auto &&conn = conn_res.value();
                conn->start();
                return cb(std::move(conn));
              });
        });
  }
}  // namespace libp2p::transport
