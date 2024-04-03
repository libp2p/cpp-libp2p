/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/converter_utils.hpp>

#include <optional>

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/endian/conversion.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>
#include <libp2p/multi/converters/dns_converter.hpp>
#include <libp2p/multi/converters/ip_v4_converter.hpp>
#include <libp2p/multi/converters/ip_v6_converter.hpp>
#include <libp2p/multi/converters/ipfs_converter.hpp>
#include <libp2p/multi/converters/tcp_converter.hpp>
#include <libp2p/multi/converters/udp_converter.hpp>
#include <libp2p/multi/multiaddress_protocol_list.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include <libp2p/multi/uvarint.hpp>

// TODO: CLANG-TIDY TEST

// TODO(turuslan): qtils, https://github.com/qdrvm/kagome/issues/1813
namespace qtils {
  inline std::string_view byte2str(const libp2p::BytesIn &s) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char *>(s.data()), s.size()};
  }
}  // namespace qtils

// https://github.com/multiformats/rust-multiaddr/blob/3c7e813c3b1fdd4187a9ca9ff67e10af0e79231d/src/protocol.rs#L613-L622
inline void percentEncode(std::string &out, std::string_view str) {
  constexpr std::array<uint32_t, 4> mask{
      0xffffffff,
      0xd000802d,
      0x00000000,
      0xa8000001,
  };
  for (const auto &c : str) {
    if (static_cast<unsigned char>(c) > 0x7f
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        or (mask[c / 32] & (1 << (c % 32))) != 0) {
      fmt::format_to(std::back_inserter(out), "%{:02X}", c);
    } else {
      out += c;
    }
  }
}

// https://github.com/multiformats/rust-multiaddr/blob/3c7e813c3b1fdd4187a9ca9ff67e10af0e79231d/src/protocol.rs#L203-L212
inline std::string percentDecode(std::string_view str) {
  auto f = [&](char c) -> std::optional<uint8_t> {
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
      return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
      return c - 'a' + 10;
    }
    return std::nullopt;
  };
  std::string out;
  while (not str.empty()) {
    if (str[0] == '%' and str.size() >= 3) {
      auto x1 = f(str[1]), x2 = f(str[2]);
      if (x1 and x2) {
        out.push_back((*x1 << 4) | *x2);
        str.remove_prefix(3);
        continue;
      }
    }
    out.push_back(str[0]);
    str.remove_prefix(1);
  }
  return out;
}

namespace libp2p::multi::converters {

  outcome::result<Bytes> multiaddrToBytes(std::string_view str) {
    if (str.empty() || str[0] != '/') {
      return ConversionError::ADDRESS_DOES_NOT_BEGIN_WITH_SLASH;
    }

    str.remove_prefix(1);
    if (str.empty()) {
      return ConversionError::EMPTY_PROTOCOL;
    }

    if (str.back() == '/') {
      // for split does not recognize an empty token in the end
      str.remove_suffix(1);
    }

    Bytes processed;

    enum class WordType { PROTOCOL, ADDRESS };
    WordType type = WordType::PROTOCOL;

    const Protocol *protx = nullptr;

    std::list<std::string> tokens;
    boost::algorithm::split(tokens, str, boost::algorithm::is_any_of("/"));

    for (auto &word : tokens) {
      if (type == WordType::PROTOCOL) {
        if (word.empty()) {
          return ConversionError::EMPTY_PROTOCOL;
        }
        protx = ProtocolList::get(word);
        if (protx != nullptr) {
          auto code = UVarint(static_cast<uint64_t>(protx->code)).toVector();
          for (auto byte : code) {
            processed.emplace_back(byte);
          }
          if (protx->size != 0) {
            type = WordType::ADDRESS;  // Since the next word will be an address
          }
        } else {
          return ConversionError::NO_SUCH_PROTOCOL;
        }
      } else {
        if (word.empty()) {
          return ConversionError::EMPTY_ADDRESS;
        }
        OUTCOME_TRY(bytes, addressToBytes(*protx, word));
        for (auto byte : bytes) {
          processed.emplace_back(byte);
        }
        protx = nullptr;  // Since right now it doesn't need that
        // assignment anymore.
        type = WordType::PROTOCOL;  // Since the next word will be an protocol
      }
    }

    if (type == WordType::ADDRESS) {
      return ConversionError::EMPTY_ADDRESS;
    }

    return processed;
  }

  outcome::result<Bytes> addressToBytes(const Protocol &protocol,
                                        std::string_view addr) {
    // TODO(Akvinikym) 25.02.19 PRE-49: add more protocols
    switch (protocol.code) {
      case Protocol::Code::IP4:
        return IPv4Converter::addressToBytes(addr);
      case Protocol::Code::IP6:
        return IPv6Converter::addressToBytes(addr);
      case Protocol::Code::TCP:
        return TcpConverter::addressToBytes(addr);
      case Protocol::Code::UDP:
        return UdpConverter::addressToBytes(addr);
      case Protocol::Code::P2P:
        return IpfsConverter::addressToBytes(addr);

      case Protocol::Code::DNS:
      case Protocol::Code::DNS4:
      case Protocol::Code::DNS6:
      case Protocol::Code::DNS_ADDR:
      case Protocol::Code::UNIX:
        return DnsConverter::addressToBytes(addr);
      case Protocol::Code::X_PARITY_WS:
      case Protocol::Code::X_PARITY_WSS:
        return DnsConverter::addressToBytes(percentDecode(addr));

      case Protocol::Code::IP6_ZONE:
      case Protocol::Code::ONION3:
      case Protocol::Code::GARLIC64:
      case Protocol::Code::QUIC:
      case Protocol::Code::WS:
      case Protocol::Code::WSS:
      case Protocol::Code::P2P_WEBSOCKET_STAR:
      case Protocol::Code::P2P_STARDUST:
      case Protocol::Code::P2P_WEBRTC_DIRECT:
      case Protocol::Code::P2P_CIRCUIT:
        return ConversionError::NOT_IMPLEMENTED;
      default:
        return ConversionError::NO_SUCH_PROTOCOL;
    }
  }

  outcome::result<std::string> bytesToMultiaddrString(BytesIn bytes) {
    auto read = [&](size_t n) -> outcome::result<BytesIn> {
      if (n > bytes.size()) {
        return ConversionError::INVALID_ADDRESS;
      }
      auto r = bytes.first(n);
      bytes = bytes.subspan(n);
      return r;
    };
    auto uvar = [&]() -> outcome::result<uint64_t> {
      auto var = UVarint::create(bytes);
      if (not var) {
        return ConversionError::INVALID_ADDRESS;
      }
      bytes = bytes.subspan(var->size());
      return var->toUInt64();
    };
    auto read_uvar = [&]() -> outcome::result<BytesIn> {
      OUTCOME_TRY(n, uvar());
      return read(n);
    };
    std::string results;
    while (not bytes.empty()) {
      OUTCOME_TRY(protocol_num, uvar());
      const Protocol *protocol =
          ProtocolList::get(static_cast<Protocol::Code>(protocol_num));
      if (protocol == nullptr) {
        return ConversionError::NO_SUCH_PROTOCOL;
      }
      results += "/";
      results += protocol->name;
      if (protocol->size == 0) {
        continue;
      }
      switch (protocol->code) {
        case Protocol::Code::P2P: {
          OUTCOME_TRY(data, read_uvar());
          results += "/";
          results += detail::encodeBase58(data);
          break;
        }

        case Protocol::Code::DNS:
        case Protocol::Code::DNS4:
        case Protocol::Code::DNS6:
        case Protocol::Code::DNS_ADDR: {
          OUTCOME_TRY(data, read_uvar());
          auto name = qtils::byte2str(data);
          const auto *i =
              std::find_if_not(name.begin(), name.end(), [](auto c) {
                return std::isalnum(c) || c == '-' || c == '.';
              });
          if (i != name.end()) {
            return ConversionError::INVALID_ADDRESS;
          }
          results += "/";
          results += name;
          break;
        }

        case Protocol::Code::X_PARITY_WS:
        case Protocol::Code::X_PARITY_WSS: {
          OUTCOME_TRY(data, read_uvar());
          results += "/";
          percentEncode(results, qtils::byte2str(data));
          break;
        }

        case Protocol::Code::IP4: {
          std::array<uint8_t, 4> arr{};
          OUTCOME_TRY(data, read(arr.size()));
          std::copy(data.begin(), data.end(), arr.begin());
          results += "/";
          results += boost::asio::ip::make_address_v4(arr).to_string();
          break;
        }

        case Protocol::Code::IP6: {
          std::array<uint8_t, 16> arr{};
          OUTCOME_TRY(data, read(arr.size()));
          std::copy(data.begin(), data.end(), arr.begin());
          results += "/";
          results += boost::asio::ip::make_address_v6(arr).to_string();
          break;
        }

        case Protocol::Code::TCP:
        case Protocol::Code::UDP: {
          OUTCOME_TRY(data, read(sizeof(uint16_t)));
          results += "/";
          results += std::to_string(boost::endian::load_big_u16(data.data()));
          break;
        }

        default:
          return ConversionError::NOT_IMPLEMENTED;
      }
    }

    return results;
  }
}  // namespace libp2p::multi::converters
