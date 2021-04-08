/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/storage/sqlite.hpp>

namespace libp2p::storage {

  SQLite::SQLite(const std::string &db_file)
      : db_(db_file),
        db_file_(db_file),
        log_(log::createLogger(kLoggerTag)) {}

  SQLite::SQLite(const std::string &db_file, const std::string &logger_tag)
      : db_(db_file),
        db_file_(db_file),
        log_(log::createLogger(logger_tag)) {}

  SQLite::~SQLite() {
    // without the following, all the prepared statements
    // might be executed when db_'s destructor is called
    for (auto &st : statements_) {
      st.used(true);
    }
  }

  int SQLite::getErrorCode() {
    return sqlite3_extended_errcode(db_.connection().get());
  }

  std::string SQLite::getErrorMessage() {
    int ec{getErrorCode()};
    return (0 == ec) ? std::string()
                     : std::string(sqlite3_errstr(ec)) + ": "
            + sqlite3_errmsg(db_.connection().get());
  }

  SQLite::StatementHandle SQLite::createStatement(const std::string &sql) {
    auto handle{statements_.size()};
    statements_.emplace_back(db_ << sql);
    return handle;
  }

  SQLite::database_binder &SQLite::getStatement(
      SQLite::StatementHandle handle) {
    if (handle >= statements_.size()) {
      throw std::runtime_error("SQLite: statement does not exist");
    }
    return statements_[handle];
  }

  int SQLite::countChanges() {
    return sqlite3_changes(db_.connection().get());
  }
}  // namespace libp2p::storage
