/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ADDRESS_REPOSITORY_MOCK_HPP
#define LIBP2P_ADDRESS_REPOSITORY_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/peer/address_repository.hpp"

namespace libp2p::peer {

  struct AddressRepositoryMock : public AddressRepository {
    ~AddressRepositoryMock() override = default;

    // address repository
    MOCK_METHOD3(addAddresses,
                 outcome::result<void>(const PeerId &,
                                       gsl::span<const multi::Multiaddress>,
                                       Milliseconds));
    MOCK_METHOD3(upsertAddresses,
                 outcome::result<void>(const PeerId &,
                                       gsl::span<const multi::Multiaddress>,
                                       Milliseconds));

    MOCK_METHOD2(updateAddresses,
                 outcome::result<void>(const PeerId &, Milliseconds));

    MOCK_CONST_METHOD1(
        getAddresses,
        outcome::result<std::vector<multi::Multiaddress>>(const PeerId &));

    MOCK_METHOD1(clear, void(const PeerId &p));

    MOCK_CONST_METHOD0(getPeers, std::unordered_set<PeerId>());

    // garbage collectable
    MOCK_METHOD0(collectGarbage, void());
  };

}  // namespace libp2p::peer
#endif  //  LIBP2P_ADDRESS_REPOSITORY_MOCK_HPP
