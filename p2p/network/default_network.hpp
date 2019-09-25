/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DEFAULT_NETWORK_HPP
#define KAGOME_DEFAULT_NETWORK_HPP

// implementations
#include "crypto/key_generator/key_generator_impl.hpp"
#include "crypto/key_marshaller/key_marshaller_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "muxer/yamux.hpp"
#include "network/impl/connection_manager_impl.hpp"
#include "network/impl/dialer_impl.hpp"
#include "network/impl/listener_manager_impl.hpp"
#include "network/impl/network_impl.hpp"
#include "network/impl/router_impl.hpp"
#include "network/impl/transport_manager_impl.hpp"
#include "peer/impl/identity_manager_impl.hpp"
#include "protocol_muxer/multiselect.hpp"
#include "security/plaintext.hpp"
#include "transport/impl/upgrader_impl.hpp"
#include "transport/tcp.hpp"

#endif //KAGOME_DEFAULT_NETWORK_HPP
