/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SQLITE_HPP
#define LIBP2P_SQLITE_HPP

#include <vector>

#include <sqlite_modern_cpp.h>
#include <libp2p/log/logger.hpp>

namespace libp2p::storage {

  /// C++ handy interface for SQLite based on SQLiteModernCpp
  class SQLite {
   public:
    using StatementHandle = size_t;
    using database_binder = ::sqlite::database_binder;
    static auto constexpr kLoggerTag = "sqlite";

    explicit SQLite(const std::string &db_file);
    SQLite(const std::string &db_file, const std::string &logger_tag);
    ~SQLite();

    template <typename T>
    auto operator<<(const T &t) {
      return (db_ << t);
    }

    template <typename T>
    SQLite &operator>>(T &t) {
      db_ >> t;
      return *this;
    }

    /// Reads extended sqlite3 error code
    int getErrorCode();

    /// Returns human-readable representation of an error
    std::string getErrorMessage();

    /**
     * Store prepared statement
     * @param sql - prepared statement body
     * @return - handle to the statement for
     */
    StatementHandle createStatement(const std::string &sql);

    /**
     * Retrieves SQLiteModernCpp prepared statement
     * @param handle - statement identifier
     * @return - statement bound to the db
     */
    database_binder &getStatement(StatementHandle handle);

    /**
     * Executes a command from a prepared statement
     * @tparam Args - command arguments' types
     * @param st_handle - statement identifier
     * @param args - command arguments
     * @return number of rows affected, -1 in case of error
     */
    template <typename... Args>
    inline int execCommand(StatementHandle st_handle,
                           const Args &... args) noexcept {
      try {
        auto &st = getStatement(st_handle);
        bindArgs(st, args...);
        st.execute();
        return countChanges();
      } catch (const std::runtime_error &e) {
        // getStatement can receive invalid handle
        log_->error(e.what());
      } catch (...) {
        log_->error(getErrorMessage());
      }
      return -1;
    }

    /**
     * Executes a query from a prepared statement
     * @tparam Sink - query response consumer type
     * @tparam Args - query arguments' types
     * @param st_handle - statement identifier
     * @param sink - query response consumer
     * @param args - query arguments
     * @return true when query was successfully executed, otherwise - false
     */
    template <typename Sink, typename... Args>
    inline bool execQuery(StatementHandle st_handle, Sink &&sink,
                          const Args &... args) noexcept {
      try {
        auto &st = getStatement(st_handle);
        bindArgs(st, args...);
        st >> sink;
        return true;
      } catch (const std::runtime_error &e) {
        // getStatement can receive invalid handle
        log_->error(e.what());
      } catch (...) {
        log_->error(getErrorMessage());
      }
      return false;
    }

   private:
    static inline database_binder &bindArgs(database_binder &statement) {
      return statement;
    }

    template <typename Arg, typename... Args>
    static inline database_binder &bindArgs(database_binder &statement,
                                            const Arg &arg,
                                            const Args &... args) {
      statement << arg;
      return bindArgs(statement, args...);
    }

    /// Returns the number of rows modified
    int countChanges();

    ::sqlite::database db_;
    std::string db_file_;
    log::Logger log_;

    std::vector<database_binder> statements_;
  };

}  // namespace libp2p::storage

#endif  // LIBP2P_SQLITE_HPP
