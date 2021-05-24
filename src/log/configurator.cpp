/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/log/configurator.hpp>

namespace libp2p::log {

  namespace {
    const std::string embedded_config(R"(
# This is libp2p configuration part of logging system
# ------------- Begin of libp2p config --------------
groups:
  - name: libp2p
    level: off
    children:
      - name: muxer
        children:
          - name: mplex
          - name: yamux
      - name: crypto
      - name: security
        children:
          - name: plaintext
          - name: secio
          - name: noise
      - name: protocols
        children:
          - name: echo
          - name: identify
          - name: kademlia
      - name: storage
        children:
          - name: sqlite
      - name: utils
        children:
          - name: ares
          - name: tls
          - name: listener_manager
          - name: libp2p_debug
          - name: scheduler
# --------------- End of libp2p config ---------------)");
  }

  Configurator::Configurator() : ConfiguratorFromYAML(embedded_config) {}

  Configurator::Configurator(std::string config)
      : soralog::ConfiguratorFromYAML(std::move(config)) {}

}  // namespace libp2p::log
