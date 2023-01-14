/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/multiaddress.hpp>

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <libp2p/multi/converters/converter_utils.hpp>

using std::string_literals::operator""s;

namespace {

  /**
   * Find all occurrences of the string in other string
   * @param string to search in
   * @param substring to be searched for
   * @return vector with positions of all occurrences of that substring
   */
  std::vector<size_t> findSubstringOccurrences(std::string_view string,
                                               std::string_view substring) {
    std::vector<size_t> occurrences;
    auto occurrence = string.find(substring);
    while (occurrence != std::string_view::npos) {
      occurrences.push_back(occurrence);
      occurrence = string.find(substring, occurrence + substring.size());
    }
    return occurrences;
  }
}  // namespace

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi, Multiaddress::Error, e) {
  using libp2p::multi::Multiaddress;
  switch (e) {
    case Multiaddress::Error::INVALID_INPUT:
      return "invalid multiaddress input";
    case Multiaddress::Error::INVALID_PROTOCOL_VALUE:
      return "protocol value can not be casted to T";
    case Multiaddress::Error::PROTOCOL_NOT_FOUND:
      return "multiaddress does not contain given protocol";
    default:
      return "unknown";
  }
}

namespace libp2p::multi {
  using std::string_literals::operator""s;

  Multiaddress::FactoryResult Multiaddress::create(std::string_view address) {
    // convert string address to bytes and make sure they represent valid
    // address
    auto bytes_result = converters::multiaddrToBytes(address);
    if (!bytes_result) {
      return bytes_result.as_failure();
    }
    auto &&bytes = bytes_result.value();

    auto str_res = converters::bytesToMultiaddrString(bytes);
    BOOST_ASSERT(str_res.has_value());
    auto &&str = str_res.value();

    return Multiaddress{
        std::move(str),
        ByteBuffer{std::vector<uint8_t>{bytes.begin(), bytes.end()}}};
  }

  Multiaddress::FactoryResult Multiaddress::create(
      gsl::span<const uint8_t> bytes) {
    auto conversion_res = converters::bytesToMultiaddrString(bytes);
    if (!conversion_res) {
      return Error::INVALID_INPUT;
    }

    std::string s = conversion_res.value();
    return Multiaddress{std::move(s), ByteBuffer(bytes.begin(), bytes.end())};
  }

  Multiaddress::FactoryResult Multiaddress::create(const ByteBuffer &bytes) {
    return create(gsl::span<const uint8_t>(bytes));
  }

  Multiaddress::Multiaddress(std::string &&address, ByteBuffer &&bytes)
      : stringified_address_{std::move(address)}, bytes_{std::move(bytes)} {}

  void Multiaddress::encapsulate(const Multiaddress &address) {
    stringified_address_ += address.stringified_address_;

    const auto &other_bytes = address.bytes_;
    bytes_.insert(bytes_.end(), other_bytes.begin(), other_bytes.end());
  }

  bool Multiaddress::decapsulate(const Multiaddress &address) {
    return decapsulateStringFromAddress(address.stringified_address_,
                                        address.bytes_);
  }

  bool Multiaddress::decapsulate(Protocol::Code proto, std::string address) {
    auto p = ProtocolList::get(proto);
    if (p == nullptr) {
      return false;
    }

    std::string proto_str = '/' + std::string(p->name);
    if (not address.empty()) {
      proto_str += '/';
      proto_str += address;
    }
    auto proto_bytes = converters::multiaddrToBytes(proto_str);
    if (!proto_bytes) {
      return false;
    }
    return decapsulateStringFromAddress(proto_str, proto_bytes.value());
  }

  std::pair<Multiaddress, boost::optional<Multiaddress>>
  Multiaddress::splitFirst() const {
    auto second_slash = stringified_address_.find('/', 1);
    if (second_slash == std::string::npos) {
      return {*this, boost::none};
    }

    auto third_slash = stringified_address_.find('/', second_slash + 1);
    if (third_slash == std::string::npos) {
      return {*this, boost::none};
    }

    // it's safe to get values in-place, as parts of Multiaddress are guaranteed
    // to be valid Multiaddresses themselves
    return {create(stringified_address_.substr(0, third_slash)).value(),
            create(stringified_address_.substr(third_slash)).value()};
  }

  bool Multiaddress::decapsulateStringFromAddress(std::string_view proto,
                                                  const ByteBuffer &bytes) {
    auto str_pos = stringified_address_.rfind(proto);
    if (str_pos == std::string::npos) {
      return false;
    }
    stringified_address_.erase(str_pos);

    const auto &this_bytes = bytes_;
    const auto &other_bytes = bytes;
    auto bytes_pos = std::find_end(this_bytes.begin(), this_bytes.end(),
                                   other_bytes.begin(), other_bytes.end());
    bytes_ = ByteBuffer{this_bytes.begin(), bytes_pos};

    return true;
  }

  std::string_view Multiaddress::getStringAddress() const {
    return stringified_address_;
  }

  const Multiaddress::ByteBuffer &Multiaddress::getBytesAddress() const {
    return bytes_;
  }

  boost::optional<std::string> Multiaddress::getPeerId() const {
    auto peer_id = getValuesForProtocol(Protocol::Code::P2P);
    if (peer_id.empty()) {
      return {};
    }
    return peer_id[0];
  }

  std::vector<std::string> Multiaddress::getValuesForProtocol(
      Protocol::Code proto) const {
    std::vector<std::string> values;
    auto protocol = ProtocolList::get(proto);
    if (protocol == nullptr) {
      return {};
    }
    auto proto_str = "/"s + std::string(protocol->name) + "/";
    auto proto_positions =
        findSubstringOccurrences(stringified_address_, proto_str);
    if (proto == Protocol::Code::P2P) {  // ipfs and p2p are equivalent
      auto ipfs_occurences =
          findSubstringOccurrences(stringified_address_, "/ipfs"s);
      proto_positions.insert(proto_positions.end(), ipfs_occurences.begin(),
                             ipfs_occurences.end());
    }

    for (const auto &pos : proto_positions) {
      auto value_pos = stringified_address_.find_first_of('/', pos + 1) + 1;
      auto value_end = stringified_address_.find_first_of('/', value_pos);
      values.push_back(
          stringified_address_.substr(value_pos, value_end - value_pos));
    }

    return values;
  }

  std::list<Protocol> Multiaddress::getProtocols() const {
    std::string_view addr{stringified_address_};
    addr.remove_prefix(1);

    std::list<std::string> tokens;

    boost::algorithm::split(tokens, addr, boost::algorithm::is_any_of("/"));

    std::list<Protocol> protocols;
    for (auto &token : tokens) {
      auto p = ProtocolList::get(token);
      if (p != nullptr) {
        protocols.emplace_back(*p);
      }
    }
    return protocols;
  }

  std::vector<std::pair<Protocol, std::string>>
  Multiaddress::getProtocolsWithValues() const {
    std::string_view addr{stringified_address_};
    addr.remove_prefix(1);
    if (addr.back() == '/') {
      addr.remove_suffix(1);
    }

    std::vector<std::string> tokens;

    boost::algorithm::split(tokens, addr, boost::algorithm::is_any_of("/"));

    std::vector<std::pair<Protocol, std::string>> pvs;
    for (auto &token : tokens) {
      auto p = ProtocolList::get(token);
      if (p != nullptr) {
        pvs.emplace_back(*p, "");
      } else {
        auto &s = pvs.back().second;
        if (!s.empty()) {
          s += "/";
        }
        s += token;
      }
    }
    return pvs;
  }

  bool Multiaddress::operator==(const Multiaddress &other) const {
    return this->stringified_address_ == other.stringified_address_
        && this->bytes_ == other.bytes_;
  }

  outcome::result<std::string> Multiaddress::getFirstValueForProtocol(
      Protocol::Code proto) const {
    // TODO(@warchant): refactor it to be more performant. this isn't best
    // solution
    auto vec = getValuesForProtocol(proto);
    if (vec.empty()) {
      return Error::PROTOCOL_NOT_FOUND;
    }

    return vec[0];
  }

  bool Multiaddress::operator<(const Multiaddress &other) const {
    return this->stringified_address_ < other.stringified_address_;
  }

  bool Multiaddress::hasProtocol(Protocol::Code code) const {
    auto p = ProtocolList::get(code);
    if (p == nullptr) {
      return false;
    }

    auto str = '/' + std::string(p->name) + '/';
    return this->stringified_address_.find(str) != std::string::npos;
  }

}  // namespace libp2p::multi

size_t std::hash<libp2p::multi::Multiaddress>::operator()(
    const libp2p::multi::Multiaddress &x) const {
  return std::hash<std::string_view>()(x.getStringAddress());
}
