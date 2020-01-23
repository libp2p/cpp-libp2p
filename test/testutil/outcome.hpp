/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GTEST_OUTCOME_UTIL_HPP
#define LIBP2P_GTEST_OUTCOME_UTIL_HPP

#include <gtest/gtest.h>
#include <libp2p/outcome/outcome.hpp>

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a##b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define EXPECT_OUTCOME_TRUE_VOID(var, expr) \
  auto && (var) = expr;                     \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " << (var).error().message();

#define EXPECT_OUTCOME_TRUE_NAME(var, val, expr)                              \
  auto && (var) = expr;                                                       \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " << (var).error().message(); \
  auto && (val) = (var).value();

#define EXPECT_OUTCOME_FALSE_VOID(var, expr) \
  auto && (var) = expr;                      \
  EXPECT_FALSE(var) << "Line " << __LINE__ << ": " << (var).error().message();

#define EXPECT_OUTCOME_FALSE_NAME(var, val, expr)                              \
  auto && (var) = expr;                                                        \
  EXPECT_FALSE(var) << "Line " << __LINE__ << ": " << (var).error().message(); \
  auto && (val) = (var).error();

#define EXPECT_OUTCOME_TRUE_3(var, val, expr) \
  EXPECT_OUTCOME_TRUE_NAME(var, val, expr)

#define EXPECT_OUTCOME_TRUE_2(val, expr) \
  EXPECT_OUTCOME_TRUE_3(UNIQUE_NAME(_r), val, expr)

#define EXPECT_OUTCOME_TRUE_1(expr) \
  EXPECT_OUTCOME_TRUE_VOID(UNIQUE_NAME(_v), expr)

#define EXPECT_OUTCOME_FALSE_3(var, val, expr) \
  EXPECT_OUTCOME_FALSE_NAME(var, val, expr)

#define EXPECT_OUTCOME_FALSE_2(val, expr) \
  EXPECT_OUTCOME_FALSE_3(UNIQUE_NAME(_r), val, expr)

#define EXPECT_OUTCOME_FALSE_1(expr) \
  EXPECT_OUTCOME_FALSE_VOID(UNIQUE_NAME(_v), expr)

/**
 * Use this macro in GTEST with 2 arguments to assert that getResult()
 * returned VALUE and immediately get this value.
 * EXPECT_OUTCOME_TRUE(val, getResult());
 */
#define EXPECT_OUTCOME_TRUE(val, expr) \
  EXPECT_OUTCOME_TRUE_NAME(UNIQUE_NAME(_r), val, expr)

#define EXPECT_OUTCOME_FALSE(val, expr) \
  EXPECT_OUTCOME_FALSE_NAME(UNIQUE_NAME(_f), val, expr)

#define EXPECT_OUTCOME_TRUE_MSG_VOID(var, expr, msg)                         \
  auto && (var) = expr;                                                      \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " << (var).error().message() \
                   << "\t" << (msg);

#define EXPECT_OUTCOME_TRUE_MSG_NAME(var, val, expr, msg)                    \
  auto && (var) = expr;                                                      \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " << (var).error().message() \
                   << "\t" << (msg);                                         \
  auto && (val) = (var).value();

/**
 * Use this macro in GTEST with 2 arguments to assert that
 * result of expression as outcome::result<T> is value and,
 * but the value itself is not necessary.
 * If result is error, macro prints corresponding error message
 * and appends custom error message specified in msg.
 */
#define EXPECT_OUTCOME_TRUE_MSG_1(expr, msg) \
  EXPECT_OUTCOME_TRUE_MSG_VOID(UNIQUE_NAME(_v), expr, msg)

/**
 * Use this macro in GTEST with 3 arguments to assert that
 * result of expression as outcome::result<T> is value and
 * immediately get access to this value.
 * If result is error, macro prints corresponding error message
 * and appends custom error message specified in msg.
 */
#define EXPECT_OUTCOME_TRUE_MSG(val, expr, msg) \
  EXPECT_OUTCOME_TRUE_MSG_NAME(UNIQUE_NAME(_r), val, expr, msg)

#endif  // LIBP2P_GTEST_OUTCOME_UTIL_HPP
