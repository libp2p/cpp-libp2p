/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <type_traits>

#include <libp2p/log/logger.hpp>

#ifndef LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP
#define LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP

namespace libp2p::log {

  /// Local logger with common prefix used to distinguish message source
  /// instances
  class SubLogger {
   public:
    explicit SubLogger(const std::string &tag, const std::string &group)
        : log_(log::createLogger(tag, group)),
          prefix_(),
          prefix_size_(prefix_.size()) {}

    explicit SubLogger(const std::string &tag, const std::string &group,
                       std::string_view prefix)
        : log_(log::createLogger(tag, group)),
          prefix_(prefix),
          prefix_size_(prefix_.size() + 1) {
      prefix_ += ' ';
    }

    template <typename T>
    explicit SubLogger(const std::string &tag, const std::string &group,
                       std::string_view prefix, T instance)
        : log_(log::createLogger(tag, group)),
          prefix_(makePrefix(prefix, instance)),
          prefix_size_(prefix_.size()) {}

    template <typename... Args>
    void log(soralog::Level level, std::string_view fmt, const Args &... args) {
      if (log_->level() >= level) {
        prefix_.append(fmt.data(), fmt.size());
        log_->log(level, prefix_, args...);
        prefix_.resize(prefix_size_);
      }
    }

    template <typename... Args>
    void trace(std::string_view fmt, const Args &... args) {
      log(Level::TRACE, fmt, args...);
    }

    template <typename... Args>
    void debug(std::string_view fmt, const Args &... args) {
      log(Level::DEBUG, fmt, args...);
    }

    template <typename... Args>
    void verbose(std::string_view fmt, const Args &... args) {
      log(Level::VERBOSE, fmt, args...);
    }

    template <typename... Args>
    void info(std::string_view fmt, const Args &... args) {
      log(Level::INFO, fmt, args...);
    }

    template <typename... Args>
    void warn(std::string_view fmt, const Args &... args) {
      log(Level::WARN, fmt, args...);
    }

    template <typename... Args>
    void error(std::string_view fmt, const Args &... args) {
      log(Level::ERROR, fmt, args...);
    }

    template <typename... Args>
    void critical(std::string_view fmt, const Args &... args) {
      log(Level::CRITICAL, fmt, args...);
    }

   private:
    template <typename T>
    auto makePrefix(std::string_view prefix, T instance) {
      if constexpr (std::is_pointer_v<
                        T> and not std::is_same_v<T, const char *>) {
        return fmt::format("{}({:x}): ", prefix,
                           reinterpret_cast<void *>(instance));  // NOLINT;
      } else if constexpr (std::is_integral_v<T> and sizeof(T) > 1) {
        return fmt::format("{}#{}: ", prefix, instance);
      } else {
        return fmt::format("{}.{}: ", prefix, instance);
      }
    }

    log::Logger log_;
    std::string prefix_;
    size_t prefix_size_ = 0;
  };
}  // namespace libp2p::log

#endif  // LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP
