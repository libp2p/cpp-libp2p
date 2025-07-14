/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include <qtils/bytes.hpp>

#define EXPECT_CALL_READ(mock) \
  EXPECT_CALL(mock, readSome(testing::_, testing::_))
#define WILL_READ(_expected)                                 \
  WillOnce([expected{qtils::asVec(_expected)}](              \
               libp2p::BytesOut out,                         \
               libp2p::basic::Reader::ReadCallbackFunc cb) { \
    ASSERT_GE(out.size(), expected.size());                  \
    memcpy(out.data(), expected.data(), expected.size());    \
    cb(expected.size());                                     \
  })
#define WILL_READ_SIZE(_expected)                                              \
  WillOnce([expected{_expected}](libp2p::BytesOut out,                         \
                                 libp2p::basic::Reader::ReadCallbackFunc cb) { \
    ASSERT_EQ(out.size(), expected);                                           \
    cb(expected);                                                              \
  })
#define WILL_READ_ERROR()                                   \
  WillOnce([](libp2p::BytesOut out,                         \
              libp2p::basic::Reader::ReadCallbackFunc cb) { \
    cb(std::errc::io_error);                                \
  })
