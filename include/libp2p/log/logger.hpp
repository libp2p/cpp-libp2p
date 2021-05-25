/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LOGGER_HPP
#define LIBP2P_LOGGER_HPP

#include <soralog/level.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>
#include <soralog/macro.hpp>

namespace libp2p::log {

  using Level = soralog::Level;
  using Logger = std::shared_ptr<soralog::Logger>;

  inline const std::string defaultGroupName("libp2p");

  void setLoggingSystem(std::shared_ptr<soralog::LoggingSystem> logging_system);

  [[nodiscard]] Logger createLogger(const std::string &tag);

  [[nodiscard]] Logger createLogger(const std::string &tag,
                                    const std::string &group);

  [[nodiscard]] Logger createLogger(const std::string &tag,
                                    const std::string &group, Level level);

  void setLevelOfGroup(const std::string &group_name, Level level);
  void resetLevelOfGroup(const std::string &group_name);

  void setLevelOfLogger(const std::string &logger_name, Level level);
  void resetLevelOfLogger(const std::string &logger_name);

}  // namespace libp2p::log

#endif  // LIBP2P_LOGGER_HPP
