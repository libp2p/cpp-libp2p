/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LOG_CONFIGURATOR
#define LIBP2P_LOG_CONFIGURATOR

#include <soralog/impl/configurator_from_yaml.hpp>

namespace libp2p::log {

  class Configurator : public soralog::ConfiguratorFromYAML {
   public:
    Configurator();

    explicit Configurator(std::string config);
  };

}  // namespace libp2p::log

#endif  // LIBP2P_LOG_CONFIGURATOR
