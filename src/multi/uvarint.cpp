/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/uvarint.hpp>

#include <libp2p/common/hexutil.hpp>

namespace libp2p::multi {
  using common::hex_upper;

  UVarint::UVarint(uint64_t number) {
    do {
      uint8_t byte = static_cast<uint8_t>(number) & 0x7f;
      number >>= 7;
      if (number != 0)
        byte |= 0x80;
      bytes_.push_back(byte);
    } while (number != 0);
  }

  UVarint::UVarint(gsl::span<const uint8_t> varint_bytes)
      : bytes_(varint_bytes.begin(),
               varint_bytes.begin() + calculateSize(varint_bytes)) {}

  UVarint::UVarint(gsl::span<const uint8_t> varint_bytes, size_t varint_size)
      : bytes_(varint_bytes.begin(), varint_bytes.begin() + varint_size) {}

  boost::optional<UVarint> UVarint::create(
      gsl::span<const uint8_t> varint_bytes) {
    size_t size = calculateSize(varint_bytes);
    if (size > 0) {
      return UVarint{varint_bytes, size};
    }
    return {};
  }

  uint64_t UVarint::toUInt64() const {
    uint64_t res = 0;
    size_t index = 0;
    for (const auto &byte : bytes_) {
      res += static_cast<uint64_t>((byte & 0x7f)) << index;
      index += 7;
    }
    return res;
  }

  gsl::span<const uint8_t> UVarint::toBytes() const {
    return gsl::span(bytes_.data(), bytes_.size());
  }

  const std::vector<uint8_t> &UVarint::toVector() const {
    return bytes_;
  }

  std::string UVarint::toHex() const {
    return hex_upper(bytes_);
  }

  size_t UVarint::size() const {
    return bytes_.size();
  }

  UVarint &UVarint::operator=(uint64_t n) {
    *this = UVarint(n);
    return *this;
  }

  bool UVarint::operator==(const UVarint &r) const {
    return std::equal(bytes_.begin(), bytes_.end(), r.bytes_.begin(),
                      r.bytes_.end());
  }

  bool UVarint::operator!=(const UVarint &r) const {
    return !(*this == r);
  }

  bool UVarint::operator<(const UVarint &r) const {
    return toUInt64() < r.toUInt64();
  }

  size_t UVarint::calculateSize(gsl::span<const uint8_t> varint_bytes) {
    size_t size = 0;
    size_t shift = 0;
    constexpr size_t capacity = sizeof(uint64_t) * 8;
    bool last_byte_found = false;
    for (const auto &byte : varint_bytes) {
      ++size;
      uint64_t slice = byte & 0x7f;
      if (shift >= capacity || slice << shift >> shift != slice) {
        size = 0;
        break;
      }
      if ((byte & 0x80) == 0) {
        last_byte_found = true;
        break;
      }
      shift += 7;
    }
    return last_byte_found ? size : 0;
  }

}  // namespace libp2p::multi
