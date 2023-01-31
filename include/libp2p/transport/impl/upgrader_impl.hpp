/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_UPGRADER_IMPL_HPP
#define LIBP2P_UPGRADER_IMPL_HPP

#include <vector>

#include <gsl/span>
#include <libp2p/layer/layer_adaptor.hpp>
#include <libp2p/muxer/muxer_adaptor.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>
#include <libp2p/security/security_adaptor.hpp>
#include <libp2p/transport/upgrader.hpp>

namespace libp2p::transport {
  class UpgraderImpl : public Upgrader,
                       public std::enable_shared_from_this<UpgraderImpl> {
    using LayerAdaptorSPtr = std::shared_ptr<layer::LayerAdaptor>;
    using SecAdaptorSPtr = std::shared_ptr<security::SecurityAdaptor>;
    using MuxAdaptorSPtr = std::shared_ptr<muxer::MuxerAdaptor>;

   public:
    /**
     * Create an instance of upgrader
     * @param protocol_muxer - protocol wrapper, allowing to negotiate about the
     * protocols with the other side
     * @param security_adaptors, which can be used to upgrade Raw connections to
     * the Secure ones
     * @param muxer_adaptors, which can be used to upgrade Secure connections to
     * the Muxed (Capable) ones
     */
    UpgraderImpl(std::shared_ptr<protocol_muxer::ProtocolMuxer> protocol_muxer,
                 std::vector<LayerAdaptorSPtr> layer_adaptors,
                 std::vector<SecAdaptorSPtr> security_adaptors,
                 std::vector<MuxAdaptorSPtr> muxer_adaptors);

    ~UpgraderImpl() override = default;

    void upgradeLayersInbound(RawSPtr conn, ProtoAddrVec layers,
                              OnLayerCallbackFunc cb) override;

    void upgradeLayersOutbound(const multi::Multiaddress &address, RawSPtr conn,
                               ProtoAddrVec layers,
                               OnLayerCallbackFunc cb) override;

    void upgradeToSecureInbound(LayerSPtr conn,
                                OnSecuredCallbackFunc cb) override;
    void upgradeToSecureOutbound(LayerSPtr conn, const peer::PeerId &remoteId,
                                 OnSecuredCallbackFunc cb) override;

    void upgradeToMuxed(SecSPtr conn, OnMuxedCallbackFunc cb) override;

    enum class Error { SUCCESS = 0, NO_ADAPTOR_FOUND = 1 };

   private:
    /**
     * Upgrade outbound connection to next layer one
     * @param conn to be upgraded
     * @param layers - protocols vector of required layers
     * @param layer_index - next layer index to update the connection
     * @param cb - callback, which is called, when a connection is upgraded or
     * error happens
     */
    void upgradeToNextLayerOutbound(const multi::Multiaddress &address,
                                    LayerSPtr conn, ProtoAddrVec layers,
                                    size_t layer_index, OnLayerCallbackFunc cb);

    /**
     * Upgrade inbound connection to next layer one
     * @param conn to be upgraded
     * @param layers - protocols vector of required layers
     * @param layer_index - next layer index to update the connection
     * @param cb - callback, which is called, when a connection is upgraded or
     * error happens
     */
    void upgradeToNextLayerInbound(LayerSPtr conn, ProtoAddrVec layers,
                                   size_t layer_index, OnLayerCallbackFunc cb);

    std::shared_ptr<protocol_muxer::ProtocolMuxer> protocol_muxer_;

    std::vector<LayerAdaptorSPtr> layer_adaptors_;
    std::vector<SecAdaptorSPtr> security_adaptors_;
    std::vector<MuxAdaptorSPtr> muxer_adaptors_;

    std::vector<peer::ProtocolName> security_protocols_;
    std::vector<peer::ProtocolName> muxer_protocols_;
  };
}  // namespace libp2p::transport

OUTCOME_HPP_DECLARE_ERROR(libp2p::transport, UpgraderImpl::Error)

#endif  // LIBP2P_UPGRADER_IMPL_HPP
