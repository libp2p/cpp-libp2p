/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXER_ADAPTOR_MOCK_HPP
#define KAGOME_MUXER_ADAPTOR_MOCK_HPP

#include <gmock/gmock.h>
#include "p2p/muxer/muxer_adaptor.hpp"

namespace libp2p::muxer {
  struct MuxerAdaptorMock : public MuxerAdaptor {
    ~MuxerAdaptorMock() override = default;

    MOCK_CONST_METHOD0(getProtocolId, peer::Protocol());

    MOCK_CONST_METHOD2(muxConnection,
                       void(std::shared_ptr<connection::SecureConnection>,
                            CapConnCallbackFunc));
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_MUXER_ADAPTOR_MOCK_HPP
