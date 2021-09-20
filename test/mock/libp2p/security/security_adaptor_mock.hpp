/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECURITY_ADAPTOR_MOCK_HPP
#define LIBP2P_SECURITY_ADAPTOR_MOCK_HPP

#include <gmock/gmock.h>
#include <libp2p/security/security_adaptor.hpp>

namespace libp2p::security {
  struct SecurityAdaptorMock : public SecurityAdaptor {
   public:
    ~SecurityAdaptorMock() override = default;

    MOCK_CONST_METHOD0(getProtocolId, peer::Protocol());

    MOCK_METHOD2(secureInbound,
                 void(std::shared_ptr<connection::RawConnection>,
                      SecConnCallbackFunc));

    MOCK_METHOD3(secureOutbound,
                 void(std::shared_ptr<connection::RawConnection>,
                      const peer::PeerId &, SecConnCallbackFunc));
  };
}  // namespace libp2p::security

#endif  // LIBP2P_SECURITY_ADAPTOR_MOCK_HPP
