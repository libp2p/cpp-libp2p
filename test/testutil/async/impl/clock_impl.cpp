/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/async/impl/clock_impl.hpp"

namespace libp2p::clock {

  template <typename ClockType>
  typename Clock<ClockType>::TimePoint ClockImpl<ClockType>::now() const {
    return ClockType::now();
  }

  template class ClockImpl<std::chrono::steady_clock>;
  template class ClockImpl<std::chrono::system_clock>;

}  // namespace libp2p::async
