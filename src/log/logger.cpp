/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/log/logger.hpp>

#include <boost/assert.hpp>

namespace libp2p::log {

  namespace {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::shared_ptr<soralog::LoggingSystem> logging_system_{};

    inline void ensure_loger_system_is_initialized() {
      BOOST_ASSERT_MSG(logging_system_,
                       "Logging system is not ready. "
                       "setLoggingSystem() must be executed once before");
    }
  }  // namespace

  void setLoggingSystem(
      std::shared_ptr<soralog::LoggingSystem> logging_system) {
    logging_system_ = std::move(logging_system);
  }

  Logger createLogger(const std::string &tag) {
    ensure_loger_system_is_initialized();
    return std::dynamic_pointer_cast<soralog::LoggerFactory>(logging_system_)
        ->getLogger(tag, defaultGroupName);
  }

  Logger createLogger(const std::string &tag, const std::string &group) {
    ensure_loger_system_is_initialized();
    return std::dynamic_pointer_cast<soralog::LoggerFactory>(logging_system_)
        ->getLogger(tag, group);
  }

  Logger createLogger(const std::string &tag, const std::string &group,
                      Level level) {
    ensure_loger_system_is_initialized();
    return std::dynamic_pointer_cast<soralog::LoggerFactory>(logging_system_)
        ->getLogger(tag, group, level);
  }

  void setLevelOfGroup(const std::string &group_name, Level level) {
    ensure_loger_system_is_initialized();
    logging_system_->setLevelOfGroup(group_name, level);
  }
  void resetLevelOfGroup(const std::string &group_name) {
    ensure_loger_system_is_initialized();
    logging_system_->resetLevelOfGroup(group_name);
  }

  void setLevelOfLogger(const std::string &logger_name, Level level) {
    ensure_loger_system_is_initialized();
    logging_system_->setLevelOfLogger(logger_name, level);
  }
  void resetLevelOfLogger(const std::string &logger_name) {
    ensure_loger_system_is_initialized();
    logging_system_->resetLevelOfLogger(logger_name);
  }

}  // namespace libp2p::log
