/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LOGGER_HPP
#define LIBP2P_LOGGER_HPP

#include <soralog/level.hpp>
#include <soralog/logger.hpp>
#include <soralog/logger_system.hpp>

namespace libp2p::log {

  using Level = soralog::Level;
  using Logger = std::shared_ptr<soralog::Logger>;

  void setLoggerSystem(std::shared_ptr<soralog::LoggerSystem> logger_system);

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
