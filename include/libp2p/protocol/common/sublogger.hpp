/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <type_traits>

#include <libp2p/log/logger.hpp>

#ifndef LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP
#define LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP

namespace libp2p::protocol {

  /// Local logger with common prefix used to distinguish message source
  /// instances
  class SubLogger {
   public:
    explicit SubLogger(const std::string &tag)
        : log_(log::createLogger(tag)) {}

    explicit SubLogger(const std::string &tag, std::string_view prefix)
        : log_(log::createLogger(tag + std::string(prefix))) {
    }

    template <typename T>
    explicit SubLogger(const std::string &tag, std::string_view prefix,
                       T instance)
        : log_(log::createLogger(tag + makePrefix(prefix, instance))) {
    }

    template <typename T>
    auto makePrefix(std::string_view prefix, T instance) {
      if constexpr (std::is_pointer_v<
          T> and not std::is_same_v<T, const char *>) {
        return fmt::format("{}({:x})", prefix, (void *)instance);  // NOLINT;
      } else if constexpr (std::is_integral_v<T> and sizeof(T) > 1) {
        return fmt::format("{}#{}", prefix, instance);
      } else {
        return fmt::format("{}.{}", prefix, instance);
      }
    }

    template <typename... Args>
    void log(soralog::Level level, std::string_view fmt,
             const Args &... args) {
      log_->template push(level, fmt, args...);
    }

    template <typename... Args>
    void trace(std::string_view fmt, const Args &... args) {
      log_->trace(fmt, args...);
    }

    template <typename... Args>
    void debug(std::string_view fmt, const Args &... args) {
      log_->template debug(fmt, args...);
    }

    template <typename... Args>
    void info(std::string_view fmt, const Args &... args) {
      log_->template info(fmt, args...);
    }

    template <typename... Args>
    void warn(std::string_view fmt, const Args &... args) {
      log_->template warn(fmt, args...);
    }

    template <typename... Args>
    void error(std::string_view fmt, const Args &... args) {
      log_->template error(fmt, args...);
    }

    template <typename... Args>
    void critical(std::string_view fmt, const Args &... args) {
      log_->template critical(fmt, args...);
    }

   private:
    log::Logger log_;
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP
