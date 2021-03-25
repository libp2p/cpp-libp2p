/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <libp2p/basic/varint_prefix_reader.hpp>
#include <libp2p/multi/uvarint.hpp>

TEST(VarintPrefixReader, VarintReadOneByOne) {
  using libp2p::multi::UVarint;
  using libp2p::basic::VarintPrefixReader;

  auto test = [](uint64_t x) {
    UVarint uvarint(x);
    auto bytes = uvarint.toBytes();
    VarintPrefixReader reader;
    for (auto b : bytes) {
      auto s = reader.consume(b);
      if (s == VarintPrefixReader::kReady) {
        EXPECT_EQ(reader.value(), x);
        break;
      }
      EXPECT_EQ(s, VarintPrefixReader::kUnderflow);
    }
  };

  test(0);
  uint64_t x = 0;
  static constexpr uint64_t max_x = 1ull << 63;

  test(max_x);

  while (x < max_x) {
    x += (x / 2) + 1;
    test(x);
  }
}

TEST(VarintPrefixReader, VarintReadFromBuffer) {
  using libp2p::multi::UVarint;
  using libp2p::basic::VarintPrefixReader;

  auto test = [](uint64_t x, gsl::span<const uint8_t> &buffer) {
    VarintPrefixReader reader;
    auto s = reader.consume(buffer);
    EXPECT_EQ(s, VarintPrefixReader::kReady);
    EXPECT_EQ(reader.value(), x);
  };

  uint64_t x = 0;
  static constexpr uint64_t max_x = 1ull << 63;
  std::vector<uint8_t> buffer;
  std::vector<uint64_t> numbers;
  while (x < max_x) {
    x += (x / 2) + 1;
    UVarint uvarint(x);
    auto bytes = uvarint.toBytes();
    buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    numbers.push_back(x);
  }

  gsl::span<const uint8_t> span(buffer);
  for (auto n : numbers) {
    test(n, span);
  }
  EXPECT_EQ(span.empty(), true);
}

TEST(VarintPrefixReader, VarintReadPartial) {
  using libp2p::multi::UVarint;
  using libp2p::basic::VarintPrefixReader;

  auto test = [](VarintPrefixReader &reader, gsl::span<const uint8_t> &buffer,
                 std::vector<uint64_t> &results) {
    if (reader.consume(buffer) == VarintPrefixReader::kReady) {
      results.push_back(reader.value());
      reader.reset();
    }
  };

  uint64_t x = std::numeric_limits<uint64_t>::max();
  std::vector<uint8_t> buffer;
  std::vector<uint64_t> numbers;
  while (x > 127) {
    UVarint uvarint(x);
    auto bytes = uvarint.toBytes();
    buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    numbers.push_back(x);
    x -= (x / 3) + 1;
  }

  std::vector<uint64_t> results;
  results.reserve(numbers.size());

  VarintPrefixReader reader;
  gsl::span<const uint8_t> whole_buffer(buffer);
  static constexpr ssize_t kFragmentSize = 5;
  while (reader.state() == VarintPrefixReader::kUnderflow
         && !whole_buffer.empty()) {
    auto fragment_size = kFragmentSize;
    if (whole_buffer.size() < fragment_size) {
      fragment_size = whole_buffer.size();
    }
    auto span = whole_buffer.first(fragment_size);
    while (reader.state() == VarintPrefixReader::kUnderflow && !span.empty()) {
      test(reader, span, results);
    }
    whole_buffer = whole_buffer.subspan(fragment_size);
  }

  EXPECT_EQ(whole_buffer.size(), 0);
  EXPECT_EQ(results, numbers);
  EXPECT_EQ(reader.state(), VarintPrefixReader::kUnderflow);
}
