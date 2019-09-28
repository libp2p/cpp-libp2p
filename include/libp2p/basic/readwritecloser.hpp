/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_READWRITECLOSER_HPP
#define LIBP2P_READWRITECLOSER_HPP

#include <libp2p/basic/closeable.hpp>
#include <libp2p/basic/readwriter.hpp>

namespace libp2p::basic {

  struct ReadWriteCloser : public ReadWriter, public Closeable {
    ~ReadWriteCloser() override = default;
  };

}  // namespace libp2p::basic

#endif  // LIBP2P_READWRITECLOSER_HPP
