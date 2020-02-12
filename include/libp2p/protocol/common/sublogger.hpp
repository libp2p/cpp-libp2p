/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <type_traits>

#include <libp2p/common/logger.hpp>

#ifndef LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP
#define LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP

namespace libp2p::protocol {

  /// Local logger with common prefix used to distinguish message source
  /// instances
  class SubLogger {
   public:
    template <typename T=int>
    explicit SubLogger(const std::string &tag,
                       spdlog::string_view_t prefix = "", T instance = T{})
        : log_(common::createLogger(tag)) {
      setInstanceName(prefix, instance);
    }

    template <typename T>
    void setInstanceName(spdlog::string_view_t prefix, T instance = T{}) {
      if constexpr (std::is_pointer<T>::value
                    and not std::is_same<T, const char *>::value) {
        prefix_ = fmt::format("{} {}: ", prefix, (void *)instance);  // NOLINT;
      } else {
        prefix_ = fmt::format("{} {}: ", prefix, instance);
      }
      prefix_size_ = prefix_.size();
    }

    template <typename... Args>
    void log(spdlog::level::level_enum lvl, spdlog::string_view_t fmt,
             const Args &... args) {
      if (log_->should_log(lvl)) {
        prefix_.append(fmt.data(), fmt.size());
        log_->log(lvl, prefix_, args...);
        prefix_.resize(prefix_size_);
      }
    }

    template <typename... Args>
    void trace(spdlog::string_view_t fmt, const Args &... args) {
      log(spdlog::level::trace, fmt, args...);
    }

    template <typename... Args>
    void debug(spdlog::string_view_t fmt, const Args &... args) {
      log(spdlog::level::debug, fmt, args...);
    }

    template <typename... Args>
    void info(spdlog::string_view_t fmt, const Args &... args) {
      log(spdlog::level::info, fmt, args...);
    }

    template <typename... Args>
    void warn(spdlog::string_view_t fmt, const Args &... args) {
      log(spdlog::level::warn, fmt, args...);
    }

    template <typename... Args>
    void error(spdlog::string_view_t fmt, const Args &... args) {
      log(spdlog::level::err, fmt, args...);
    }

    template <typename... Args>
    void critical(spdlog::string_view_t fmt, const Args &... args) {
      log(spdlog::level::critical, fmt, args...);
    }

   private:
    common::Logger log_;
    std::string prefix_;
    size_t prefix_size_ = 0;
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_COMMON_SUBLOGGER_HPP
