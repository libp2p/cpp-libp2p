/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multistream.hpp>

#include <algorithm>
#include <iterator>
#include <string_view>

#include <libp2p/multi/uvarint.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi, Multistream::Error, e) {
  using E = libp2p::multi::Multistream::Error;
  switch (e) {
    case E::NEW_LINE_EXPECTED:
      return "new line delimiter is not found";
    case E::NEW_LINE_NOT_EXPECTED:
      return "codec path must not contain the new line symbol";
    case E::SLASH_EXPECTED:
      return "codec path must begin with '/'";
    case E::WRONG_DATA_SIZE:
      return "data size specified in the multistream header is not equal to "
             "the actual size";
    case E::PREFIX_ILL_FORMATTED:
      return "prefix must not contain line breaks or forward slashes, as it is "
             "a part of the protocol URI";
    case E::REMOVE_LEAVES_EMPTY_PATH:
      return "Attempt to remove the only part of the path; empty protocol path "
             "is prohibited";
    case E::PREFIX_NOT_FOUND:
      return "prefix to be removed is not found in the protocol path";
  }
  return "unknown error";
}

namespace libp2p::multi {
  using common::ByteArray;
  using libp2p::multi::UVarint;

  Multistream::Multistream(Path codecPath, const ByteArray &data)
      : protocol_path_{std::move(codecPath)} {
    initBuffer(protocol_path_, data);
    initData();
  }

  outcome::result<Multistream> Multistream::create(Path protocol_path,
                                                   const ByteArray &data) {
    if (protocol_path.find('\n') != std::string::npos) {
      return Error::NEW_LINE_NOT_EXPECTED;
    }
    if (protocol_path.front() != '/') {
      return Error::SLASH_EXPECTED;
    }
    return Multistream{std::move(protocol_path), data};
  }

  outcome::result<Multistream> Multistream::create(const ByteArray &bytes) {
    size_t varint_length = UVarint::calculateSize(bytes);
    UVarint size(bytes);

    auto path_end = std::find(bytes.begin() + varint_length, bytes.end(), '\n');
    if (path_end == bytes.end()) {
      return Error::NEW_LINE_EXPECTED;
    }
    if (static_cast<uint64_t>(
            std::distance(bytes.begin() + varint_length, bytes.end()))
        != size.toUInt64()) {
      return Error::WRONG_DATA_SIZE;
    }

    Multistream m;
    std::copy(bytes.begin() + varint_length, path_end,
              std::back_inserter(m.protocol_path_));
    m.multistream_buffer_ = bytes;
    m.initData();
    return m;
  }

  outcome::result<std::reference_wrapper<Multistream>> Multistream::addPrefix(
      std::string_view prefix) {
    if (prefix.empty() || prefix.find("\n") != std::string::npos
        || prefix.find("/") != std::string::npos) {
      return Error::PREFIX_ILL_FORMATTED;
    }
    protocol_path_.insert(std::begin(protocol_path_), prefix.begin(),
                          prefix.end());
    protocol_path_.insert(std::begin(protocol_path_), '/');
    auto new_length = UVarint(prefix.length() + 1 + data_.size());

    initBuffer(protocol_path_, data_);
    initData();
    return std::ref(*this);
  }

  outcome::result<std::reference_wrapper<Multistream>>
  Multistream::removePrefix(std::string_view prefix) {
    if (prefix.empty() or prefix.find("\n") != std::string::npos
        or prefix.find("/") != std::string::npos) {
      return Error::PREFIX_ILL_FORMATTED;
    }

    auto prefix_pos = protocol_path_.find(prefix);
    if (prefix_pos == std::string::npos) {
      return Error::PREFIX_NOT_FOUND;
    }

    if (prefix.size() == protocol_path_.size() - 1) {
      return Error::REMOVE_LEAVES_EMPTY_PATH;
    }

    // -1/+1 because / preceding the prefix must also be removed
    protocol_path_.erase(prefix_pos - 1, prefix.size() + 1);

    initBuffer(protocol_path_, data_);
    initData();
    return std::ref(*this);
  }

  const Multistream::Path &Multistream::getProtocolPath() const {
    return protocol_path_;
  }

  gsl::span<const uint8_t> Multistream::getEncodedData() const {
    return data_;
  }

  const ByteArray &Multistream::getBuffer() const {
    return multistream_buffer_;
  }

  void Multistream::initBuffer(std::string_view protocol_path,
                               gsl::span<const uint8_t> data) {
    auto new_length = UVarint(protocol_path.length() + 1 + data.size());

    multistream_buffer_ = ByteArray();
    auto new_length_bytes = new_length.toBytes();
    multistream_buffer_.insert(multistream_buffer_.end(),
                               new_length_bytes.begin(),
                               new_length_bytes.end());
    multistream_buffer_.insert(multistream_buffer_.end(), protocol_path.begin(),
                               protocol_path.end());
    multistream_buffer_.push_back('\n');
    multistream_buffer_.insert(multistream_buffer_.end(), data.begin(),
                               data.end());
  }

  void Multistream::initData() {
    auto length = UVarint(protocol_path_.length() + 1 + data_.size());

    auto data_begin = length.size() + protocol_path_.length() + 1;
    auto it = multistream_buffer_.begin();
    std::advance(it, data_begin);
    data_ = gsl::span<const uint8_t>(it.base(),
                                     multistream_buffer_.size() - data_begin);
  }

  bool Multistream::operator==(const Multistream &other) const {
    return this->multistream_buffer_ == other.multistream_buffer_;
  }

}  // namespace libp2p::multi
