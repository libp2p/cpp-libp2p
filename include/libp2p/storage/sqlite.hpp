/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include <sqlite_modern_cpp.h>
#include <libp2p/log/logger.hpp>

namespace libp2p::storage {

  /// C++ handy interface for SQLite based on SQLiteModernCpp
  class SQLite {
   public:
    using StatementHandle = size_t;
    using database_binder = ::sqlite::database_binder;
    static constexpr auto kLoggerTag = "sqlite";

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
    int getErrorCode() const;

    /// Returns human-readable representation of an error
    std::string getErrorMessage() const;

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
     * @throws std::invalid_argument if statement handle is invalid
     */
    template <typename... Args>
    inline int execCommand(StatementHandle st_handle, const Args &...args) {
      try {
        auto &st = getStatement(st_handle);
        bindArgs(st, args...);
        st.execute();
        return countChanges();
      } catch (const std::invalid_argument &e) {
        // getStatement can receive invalid handle
        log_->error("Invalid statement handle: {}", e.what());
        throw; // Re-throw invalid_argument as it's a programming error
      } catch (const std::runtime_error &e) {
        log_->error("Runtime error during command execution: {}", e.what());
      } catch (...) {
        log_->error("Unknown error during command execution: {}", getErrorMessage());
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
     * @throws std::invalid_argument if statement handle is invalid
     */
    template <typename Sink, typename... Args>
    inline bool execQuery(StatementHandle st_handle,
                          Sink &&sink,
                          const Args &...args) {
      try {
        auto &st = getStatement(st_handle);
        bindArgs(st, args...);
        st >> sink;
        return true;
      } catch (const std::invalid_argument &e) {
        // getStatement can receive invalid handle
        log_->error("Invalid statement handle: {}", e.what());
        throw; // Re-throw invalid_argument as it's a programming error
      } catch (const std::runtime_error &e) {
        log_->error("Runtime error during query execution: {}", e.what());
      } catch (...) {
        log_->error("Unknown error during query execution: {}", getErrorMessage());
      }
      return false;
    }

   private:
    inline static database_binder &bindArgs(database_binder &statement) {
      return statement;
    }

    template <typename Arg, typename... Args>
    inline static database_binder &bindArgs(database_binder &statement,
                                            const Arg &arg,
                                            const Args &...args) {
      statement << arg;
      return bindArgs(statement, args...);
    }

    /// Returns the number of rows modified
    int countChanges() const;

    /// Returns the database file path
    const std::string &getDatabaseFile() const;

    /// Returns the number of prepared statements
    size_t getStatementCount() const;

    ::sqlite::database db_;
    std::string db_file_;
    log::Logger log_;

    std::vector<database_binder> statements_;
  };

}  // namespace libp2p::storage
