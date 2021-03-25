/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTISELECT_SERIALIZING_HPP
#define LIBP2P_MULTISELECT_SERIALIZING_HPP

#include <boost/container/static_vector.hpp>

#include "common.hpp"

namespace libp2p::protocol_muxer::multiselect::detail {

  /// Static vector for temp msg crafting
  using TmpMsgBuf =
  boost::container::static_vector<uint8_t,
      kMaxMessageSize + kMaxVarintSize>;

  /// Appends varint prefix to buffer
  template <typename Buffer>
  inline void appendVarint(Buffer &buffer, size_t size) {
    do {
      uint8_t byte = size & 0x7F;
      size >>= 7;
      if (size != 0) {
        byte |= 0x80;
      }
      buffer.push_back(byte);
    } while (size > 0);
  }

  /// Appends protocol message to buffer
  template <typename Buffer, typename String>
  inline bool appendProtocol(Buffer &buffer, const String &protocol) {
    auto msg_size = protocol.size() + 1;
    if (msg_size > kMaxMessageSize) {
      return false;
    }
    appendVarint(buffer, msg_size);
    buffer.insert(buffer.end(), protocol.begin(), protocol.end());
    buffer.push_back(kNewLine);
    return (buffer.size() <= kMaxMessageSize);
  }

  /// Creates simple protocol message (one string)
  template <typename String>
  inline MsgBuf createMessage(const String &protocol) {
    MsgBuf ret;
    ret.reserve(protocol.size() + 1 + kMaxVarintSize);
    if (!appendProtocol(ret, protocol)) {
      ret.clear();
    }
    return ret;
  }

  /// Creates complex protocol message (multiple strings)
  template <class Container>
  inline MsgBuf createMessage(const Container &protocols, bool nested) {
    MsgBuf ret_buf;
    TmpMsgBuf tmp_buf;
    bool overflow = false;

    try {
      if (nested) {
        for (const auto &p : protocols) {
          if (!appendProtocol(tmp_buf, p)) {
            overflow = true;
            break;
          }
        }
        if (!overflow) {
          tmp_buf.push_back(kNewLine);
        }
      } else {
        for (const auto &p : protocols) {
          tmp_buf.insert(tmp_buf.end(), p.begin(), p.end());
          tmp_buf.push_back(kNewLine);
        }
      }

    } catch (const std::bad_alloc &e) {
      // static tmp buffer throws this on oversize
      overflow = true;
    }

    if (!overflow) {
      ret_buf.reserve(tmp_buf.size() + kMaxVarintSize);
      appendVarint(ret_buf, tmp_buf.size());
      ret_buf.insert(ret_buf.end(), tmp_buf.begin(), tmp_buf.end());
    }

    return ret_buf;
  }

}  // namespace libp2p::protocol_muxer::mutiselect::detail

#endif  // LIBP2P_MULTISELECT_SERIALIZING_HPP
