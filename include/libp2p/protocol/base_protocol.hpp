/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASE_PROTOCOL_HPP
#define LIBP2P_BASE_PROTOCOL_HPP

#include <libp2p/basic/adaptor.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/connection/stream_and_protocol.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::protocol {

  /**
   * @brief Base class for user-defined protocols.
   *
   * @example
   * {@code}
   * struct EchoProtocol: public BaseProtocol {
   *   ...
   * };
   *
   * std::shared_ptr<Network> nw = std::make_shared<NetworkImpl>(...);
   * std::shared_ptr<BaseProtocol> p = std::make_shared<EchoProtocol>();
   *
   * // register protocol handler (server side callback will be executed
   * // when client opens a stream to us)
   * nw->addProtocol(p);
   * {@nocode}
   */
  struct BaseProtocol : public basic::Adaptor {
    ~BaseProtocol() override = default;

    /**
     * @brief Handler that is executed on responder (server) side of the
     * protocol.
     * @param stream_res, which was received
     */
    virtual void handle(StreamAndProtocol stream) = 0;
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_BASE_PROTOCOL_HPP
