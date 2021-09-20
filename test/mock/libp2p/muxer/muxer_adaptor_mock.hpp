/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MUXER_ADAPTOR_MOCK_HPP
#define LIBP2P_MUXER_ADAPTOR_MOCK_HPP

#include <libp2p/muxer/muxer_adaptor.hpp>

#include <gmock/gmock.h>

namespace libp2p::muxer {
  struct MuxerAdaptorMock : public MuxerAdaptor {
    ~MuxerAdaptorMock() override = default;

    MOCK_CONST_METHOD0(getProtocolId, peer::Protocol());

    MOCK_CONST_METHOD2(muxConnection,
                       void(std::shared_ptr<connection::SecureConnection>,
                            CapConnCallbackFunc));
  };
}  // namespace libp2p::muxer

#endif  // LIBP2P_MUXER_ADAPTOR_MOCK_HPP
