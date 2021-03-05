/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/log/logger.hpp>

#include <boost/assert.hpp>

namespace libp2p::log {

  namespace {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::shared_ptr<soralog::LoggerSystem> logger_system_;

    inline void ensure_loger_system_is_initialized() {
      BOOST_ASSERT_MSG(logger_system_,
                       "Logging system is not ready. "
                       "setLoggerSystem() must be executed once before");
    }
  }  // namespace

  void setLoggerSystem(std::shared_ptr<soralog::LoggerSystem> logger_system) {
    logger_system_ = std::move(logger_system);
  }

  [[nodiscard]] Logger createLogger(const std::string &tag) {
    ensure_loger_system_is_initialized();
    return logger_system_->getLogger(tag, "*", {}, {});
  }

  [[nodiscard]] Logger createLogger(const std::string &tag,
                                    const std::string &group) {
    ensure_loger_system_is_initialized();
    return logger_system_->getLogger(tag, group, {}, {});
  }

  [[nodiscard]] Logger createLogger(const std::string &tag,
                                    const std::string &group, Level level) {
    ensure_loger_system_is_initialized();
    return logger_system_->getLogger(tag, group, {}, level);
  }

  void setLevelOfGroup(const std::string &group_name, Level level) {
    ensure_loger_system_is_initialized();
    logger_system_->setLevelOfGroup(group_name, level);
  }
  void resetLevelOfGroup(const std::string &group_name) {
    ensure_loger_system_is_initialized();
    logger_system_->resetLevelOfGroup(group_name);
  }

  void setLevelOfLogger(const std::string &logger_name, Level level) {
    ensure_loger_system_is_initialized();
    logger_system_->setLevelOfLogger(logger_name, level);
  }
  void resetLevelOfLogger(const std::string &logger_name) {
    ensure_loger_system_is_initialized();
    logger_system_->resetLevelOfLogger(logger_name);
  }

}  // namespace libp2p::log
