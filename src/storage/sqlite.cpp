/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/storage/sqlite.hpp>

#ifdef SQLITE_ENABLED

namespace libp2p::storage {

  SQLite::SQLite(const std::string &db_file)
      : db_(db_file), db_file_(db_file), log_(log::createLogger(kLoggerTag)) {}

  SQLite::SQLite(const std::string &db_file, const std::string &logger_tag)
      : db_(db_file), db_file_(db_file), log_(log::createLogger(logger_tag)) {}

  SQLite::~SQLite() {
    // without the following, all the prepared statements
    // might be executed when db_'s destructor is called
    for (auto &st : statements_) {
      st.used(true);
    }
  }

  int SQLite::getErrorCode() const {
    return sqlite3_extended_errcode(db_.connection().get());
  }

  std::string SQLite::getErrorMessage() const {
    const int ec{getErrorCode()};
    return (0 == ec) ? std::string()
                     : std::string(sqlite3_errstr(ec)) + ": "
                           + sqlite3_errmsg(db_.connection().get());
  }

  SQLite::StatementHandle SQLite::createStatement(const std::string &sql) {
    try {
      auto handle{statements_.size()};
      statements_.emplace_back(db_ << sql);
      log_->debug("Created prepared statement {}: {}", handle, sql);
      return handle;
    } catch (const std::exception &e) {
      log_->error("Failed to create prepared statement for SQL: {} - Error: {}", sql, e.what());
      throw;
    }
  }

  SQLite::database_binder &SQLite::getStatement(
      SQLite::StatementHandle handle) {
    if (handle >= statements_.size()) {
      const auto max_handle = statements_.empty() ? 0 : statements_.size() - 1;
      throw std::invalid_argument("SQLite: statement handle " + 
                                 std::to_string(handle) + 
                                 " does not exist (max: " + 
                                 std::to_string(max_handle) + ")");
    }
    return statements_[handle];
  }

  int SQLite::countChanges() const {
    return sqlite3_changes(db_.connection().get());
  }

  const std::string &SQLite::getDatabaseFile() const {
    return db_file_;
  }

  size_t SQLite::getStatementCount() const {
    return statements_.size();
  }
}  // namespace libp2p::storage

#endif  // SQLITE_ENABLED
