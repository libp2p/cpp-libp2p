/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE
#define LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE

#include <vector>
#include <gsl/span>

#include <boost/optional.hpp>

namespace libp2p::protocol::kademlia {

/// DHT key. Contains sha256 hash of Key bytes in CIDv0 format
struct ContentValue {
		std::vector<uint8_t> data;

		/// Ctor for consistency. Data will contain sha256 of empty input
		ContentValue();

		explicit ContentValue(const std::string &str);

		explicit ContentValue(const std::vector<uint8_t> &v);

		// TODO(artem): use gsl::span<const T> + std::enable_if (integral types)
		ContentValue(const void *bytes, size_t size);

		bool operator<(const ContentValue &x) const {
			return data < x.data;
		}

		bool operator==(const ContentValue &x) const {
			return data == x.data;
		}

		/// Validates and stores CID received from the network
		static boost::optional<ContentValue> fromWire(
				const std::string &s);

		static boost::optional<ContentValue> fromWire(
				gsl::span<const uint8_t> bytes);

private:
		struct FromWire {};
		ContentValue(FromWire, std::vector<uint8_t> v);
};

} //namespace libp2p::protocol::kademlia

namespace std {
template <>
struct hash<libp2p::protocol::kademlia::ContentValue> {
		std::size_t operator()(
				const libp2p::protocol::kademlia::ContentValue &x) const;
};
}  // namespace std

#endif  // LIBP2P_KADEMLIA_KADEMLIA_CONTENTVALUE
