/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/logger.hpp>

#ifndef LIBP2P_PROTOCOL_COMMON_HELPERS_HPP
#define LIBP2P_PROTOCOL_COMMON_HELPERS_HPP

namespace libp2p::protocol {

  /// Local logger with common prefix
  class SubLogger {
   public:
    template <typename T>
    SubLogger(
        const std::string& tag, std::string prefix,
        T* instance = nullptr) :
      log_(common::createLogger(tag)),
      prefix_(std::move(prefix))
    {
      if (instance != nullptr) {
        // helper used to distinguish instances
        prefix_ += fmt::format(" {}: ", (void*)instance); //NOLINT;
      } else {
        prefix_ += ": ";
      }
      prefix_size_ = prefix_.size();
    }

    template<typename... Args>
    void log(
        spdlog::level::level_enum lvl,
        spdlog::string_view_t fmt,
        const Args &... args)
    {
      if (log_->should_log(lvl)) {
        prefix_.append(fmt.data(), fmt.size());
        log_->log(lvl, prefix_, args...);
        prefix_.resize(prefix_size_);
      }
    }

    template<typename... Args>
    void trace(spdlog::string_view_t fmt, const Args &... args)
    {
      log(spdlog::level::trace, fmt, args...);
    }

    template<typename... Args>
    void debug(spdlog::string_view_t fmt, const Args &... args)
    {
      log(spdlog::level::debug, fmt, args...);
    }

    template<typename... Args>
    void info(spdlog::string_view_t fmt, const Args &... args)
    {
      log(spdlog::level::info, fmt, args...);
    }

    template<typename... Args>
    void warn(spdlog::string_view_t fmt, const Args &... args)
    {
      log(spdlog::level::warn, fmt, args...);
    }

    template<typename... Args>
    void error(spdlog::string_view_t fmt, const Args &... args)
    {
      log(spdlog::level::err, fmt, args...);
    }

    template<typename... Args>
    void critical(spdlog::string_view_t fmt, const Args &... args)
    {
      log(spdlog::level::critical, fmt, args...);
    }

   private:
    common::Logger log_;
    std::string prefix_;
    size_t prefix_size_;
  };

} //namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_COMMON_HELPERS_HPP
