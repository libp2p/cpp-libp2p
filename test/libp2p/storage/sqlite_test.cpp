/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/storage/sqlite.hpp>

#include <vector>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "testutil/prepare_loggers.hpp"

/// Fixture for in-memory SQLite tests
struct SQLite : public ::testing::Test {
  void SetUp() override {
    testutil::prepareLoggers();

    sql = std::make_shared<libp2p::storage::SQLite>(":memory:");
  }

  std::shared_ptr<libp2p::storage::SQLite> sql;
};

/// Operators << and >> provide raw access to SQLite
TEST_F(SQLite, RawOperators) {
  int result = 0;
  *sql << "SELECT 1+1" >> result;
  ASSERT_EQ(result, 2);
}

/**
 * Preapared statement can be used more than once
 * @given a database with a table
 * @when the table is filled in with multiple calls of a prepared statement
 * @then table contents can be queries with another prepared statement more than
 * once
 */
TEST_F(SQLite, MultipleUseOfPreparedStatement) {
  *sql << "create table countable(num integer, char text);";
  auto handle =
      sql->createStatement("insert into countable(num, char) values(?, ?)");
  const char chars[] = {"abcdef"};
  for (size_t i = 0; i < sizeof(chars); ++i) {
    sql->execCommand(handle, i, chars[i]);
  }

  auto query = sql->createStatement("select sum(num) from countable");
  auto checker = [](int sum) {
    ASSERT_EQ(sum, 21);  // 21 is sum of numbers from 1 to 6
  };
  sql->execQuery(query, checker);
  sql->execQuery(query, checker);
}

/// Fixture for tests which require a file being saved on disk
struct Persistence : public ::testing::Test {
  Persistence() : kTestDbFile("test_db.sqlite") {}

  ~Persistence() {
    // this code will be executed even if test has thrown an exception
    if (boost::filesystem::exists(kTestDbFile)) {
      boost::filesystem::remove(kTestDbFile);
    }
  }

  std::string char2string(const char c) {
    return std::string(1, c);
  }

  const std::string kTestDbFile;
};

/**
 * Database saved on disk preserves stored state
 * @given an SQLite wrapper which saves the db to kTestDbFile
 * @when the db is filled with the data and gets closed
 * @then the db file can be opened and the data is still available
 */
TEST_F(Persistence, StatePreserved) {
  const char chars[] = "abcdef";
  {
    libp2p::storage::SQLite sql1(kTestDbFile);
    sql1 << "create table countable(num integer, char text);";
    auto handle =
        sql1.createStatement("insert into countable(num, char) values(?, ?)");

    // loop up to sizeof -1 because of null terminator of C-string
    for (size_t i = 0; i < sizeof(chars) - 1; ++i) {
      sql1.execCommand(handle, i, char2string(chars[i]));
    }
  }
  {
    libp2p::storage::SQLite sql2(kTestDbFile);
    auto checker = [this, chars](int number, std::string c) {
      ASSERT_GE(number, 0);
      ASSERT_LT(number, sizeof(chars));
      ASSERT_EQ(c, char2string(chars[number]));
    };
    sql2 << "select num, char from countable" >> checker;
  }
}
