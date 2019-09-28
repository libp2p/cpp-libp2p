/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>
#include <string>

using std::string_literals::operator""s;

#define ILLEGAL_CHAR_MSG "illegal char"s
#define DIV_0_MSG "division by 0"s

enum class ConversionErrc {
  SUCCESS = 0,       // 0 should not represent an error
  EMPTY_STRING = 1,  // (for rationale, see tutorial on error codes)
  ILLEGAL_CHAR = 2,
  TOO_LONG = 3,
};

namespace sooper::loong::ns {
  enum class DivisionErrc {
    DIVISION_BY_ZERO = 1,
  };
}

OUTCOME_HPP_DECLARE_ERROR(ConversionErrc)
OUTCOME_CPP_DEFINE_CATEGORY(ConversionErrc, e) {
  switch (e) {
    case ConversionErrc::SUCCESS:
      return "success";
    case ConversionErrc::EMPTY_STRING:
      return "empty string";
    case ConversionErrc::ILLEGAL_CHAR:
      return ILLEGAL_CHAR_MSG;
    case ConversionErrc::TOO_LONG:
      return "too long";
    default:
      return "unknown";
  }
}

OUTCOME_HPP_DECLARE_ERROR(sooper::loong::ns, DivisionErrc)
OUTCOME_CPP_DEFINE_CATEGORY(sooper::loong::ns, DivisionErrc, e) {
  using sooper::loong::ns::DivisionErrc;
  switch (e) {
    case DivisionErrc::DIVISION_BY_ZERO:
      return "division by 0";
    default:
      return "unknown";
  }
}

outcome::result<int> convert(const std::string &str) {
  if (str.empty())
    return ConversionErrc::EMPTY_STRING;

  if (!std::all_of(str.begin(), str.end(), ::isdigit))
    return ConversionErrc::ILLEGAL_CHAR;

  if (str.length() > 9)
    return ConversionErrc::TOO_LONG;

  return atoi(str.c_str());
}

outcome::result<int> divide(int a, int b) {
  using sooper::loong::ns::DivisionErrc;
  if (b == 0)
    return DivisionErrc::DIVISION_BY_ZERO;

  return a / b;
}

outcome::result<int> convert_and_divide(const std::string &a,
                                        const std::string &b) {
  OUTCOME_TRY(valA, convert(a));
  OUTCOME_TRY(valB, convert(b));
  OUTCOME_TRY(valDiv, divide(valA, valB));
  return valDiv;
}

/**
 * @given valid arguments for convert_and_divide
 * @when execute method which returns result
 * @then returns value
 */
TEST(Outcome, CorrectCase) {
  auto r = convert_and_divide("500", "2");
  ASSERT_TRUE(r);
  auto &&val = r.value();
  ASSERT_EQ(val, 250);
}

/**
 * @given arguments to cause conversion error for convert_and_divide
 * @when execute method which returns result
 * @then returns error
 */
TEST(Outcome, ConversionError) {
  auto r = convert_and_divide("500", "a");
  ASSERT_FALSE(r);
  auto &&err = r.error();
  ASSERT_EQ(err.message(), ILLEGAL_CHAR_MSG);
}

/**
 * @given arguments to cause division error for convert_and_divide
 * @when execute method which returns result
 * @then returns error
 */
TEST(Outcome, DivisionError) {
  auto r = convert_and_divide("500", "0");
  ASSERT_FALSE(r);
  auto &&err = r.error();
  ASSERT_EQ(err.message(), DIV_0_MSG);  // name of the enum
  using sooper::loong::ns::DivisionErrc;
  ASSERT_EQ(err.category().name(), typeid(DivisionErrc).name());
}
