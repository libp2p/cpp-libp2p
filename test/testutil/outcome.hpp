/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

#define EXPECT_OUTCOME_TRUE_VOID(var, expr)       \
  auto && (var) = expr;                           \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " \
                   << fmt::to_string(var.error());

#define EXPECT_OUTCOME_TRUE_NAME(var, val, expr) \
  EXPECT_OUTCOME_TRUE_VOID(var, expr)            \
  auto && (val) = (var).value();

#define EXPECT_OUTCOME_FALSE_VOID(var, expr) \
  auto && (var) = expr;                      \
  EXPECT_FALSE(var) << "Line " << __LINE__;

#define EXPECT_OUTCOME_TRUE_1(expr) \
  EXPECT_OUTCOME_TRUE_VOID(OUTCOME_UNIQUE, expr)

#define EXPECT_OUTCOME_FALSE_1(expr) \
  EXPECT_OUTCOME_FALSE_VOID(OUTCOME_UNIQUE, expr)

/**
 * Use this macro in GTEST with 2 arguments to assert that getResult()
 * returned VALUE and immediately get this value.
 * EXPECT_OUTCOME_TRUE(val, getResult());
 */
#define EXPECT_OUTCOME_TRUE(val, expr) \
  EXPECT_OUTCOME_TRUE_NAME(OUTCOME_UNIQUE, val, expr)

#define _EXPECT_EC(tmp, expr, expected) \
  auto &&tmp = expr;                    \
  EXPECT_TRUE(tmp.has_error());         \
  EXPECT_EQ(tmp.error(), make_error_code(expected));
#define EXPECT_EC(expr, expected) _EXPECT_EC(OUTCOME_UNIQUE, expr, expected)
