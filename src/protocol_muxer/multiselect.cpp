/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/multiselect_instance.hpp>
#include <libp2p/log/logger.hpp>

namespace libp2p::protocol_muxer::multiselect {

  namespace {
    const log::Logger &log() {
      static log::Logger logger = log::createLogger("multiselect");
      return logger;
    }

    constexpr size_t kMaxCacheSize = 8;
  }  // namespace

  void Multiselect::selectOneOf(gsl::span<const peer::Protocol> protocols,
                                std::shared_ptr<basic::ReadWriter> connection,
                                bool is_initiator, bool negotiate_multiselect,
                                ProtocolHandlerFunc cb) {
    getInstance()->selectOneOf(protocols, std::move(connection), is_initiator,
                               negotiate_multiselect, std::move(cb));
  }

  void Multiselect::instanceClosed(Instance instance,
                                   const ProtocolHandlerFunc &cb,
                                   outcome::result<peer::Protocol> result) {
    active_instances_.erase(instance);
    if (cache_.size() < kMaxCacheSize) {
      cache_.emplace_back(std::move(instance));
    }
    cb(std::move(result));
  }

  Multiselect::Instance Multiselect::getInstance() {
    Instance instance;
    if (cache_.empty()) {
      instance = std::make_shared<MultiselectInstance>(*this);
    } else {
      SL_TRACE(log(), "cache: {}->{}, active {}->{}", cache_.size(),
               cache_.size() - 1, active_instances_.size(),
               active_instances_.size() + 1);
      instance = std::move(cache_.back());
      cache_.pop_back();
    }
    active_instances_.insert(instance);
    return instance;
  }

}  // namespace libp2p::protocol_muxer::multiselect
