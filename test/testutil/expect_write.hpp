/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include <qtils/bytes.hpp>

#define EXPECT_CALL_WRITE(mock) \
  EXPECT_CALL(mock, writeSome(testing::_, testing::_, testing::_))
#define WILL_WRITE(_expected)                                 \
  WillOnce([expected{qtils::asVec(_expected)}](               \
               libp2p::BytesIn in,                            \
               size_t ambigous_size,                          \
               libp2p::basic::Writer::WriteCallbackFunc cb) { \
    ASSERT_EQ(in.size(), ambigous_size);                      \
    ASSERT_EQ(qtils::asVec(in), expected);                    \
    cb(in.size());                                            \
  })
#define WILL_WRITE_SIZE(_expected)                                         \
  WillOnce(                                                                \
      [expected{_expected}](libp2p::BytesIn in,                            \
                            size_t ambigous_size,                          \
                            libp2p::basic::Writer::WriteCallbackFunc cb) { \
        ASSERT_EQ(in.size(), ambigous_size);                               \
        ASSERT_EQ(in.size(), expected);                                    \
        cb(expected);                                                      \
      })
#define WILL_WRITE_ERROR()                                   \
  WillOnce([](libp2p::BytesIn in,                            \
              size_t ambigous_size,                          \
              libp2p::basic::Writer::WriteCallbackFunc cb) { \
    ASSERT_EQ(in.size(), ambigous_size);                     \
    cb(std::errc::io_error);                                 \
  })
