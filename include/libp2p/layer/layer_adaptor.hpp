/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LAYER_LAYERADAPTOR
#define LIBP2P_LAYER_LAYERADAPTOR

#include <memory>

#include <libp2p/basic/adaptor.hpp>
#include <libp2p/connection/layer_connection.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::layer {
  /**
   * Strategy to upgrade connections to next-layer
   */
  struct LayerAdaptor : public basic::Adaptor {
    using LayerConnCallbackFunc = std::function<void(
        outcome::result<std::shared_ptr<connection::LayerConnection>>)>;

    ~LayerAdaptor() override = default;

    /**
     * Make a next-layer connection from the current-layer one, using this
     * adaptor
     * @param conn - connection to be upgraded
     * @param cb - callback with an upgraded connection or error
     */
    virtual void upgradeConnection(
        std::shared_ptr<connection::LayerConnection> conn,
        LayerConnCallbackFunc cb) const = 0;
  };
}  // namespace libp2p::layer

#endif  // LIBP2P_LAYER_LAYERADAPTOR
