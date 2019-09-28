/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CLOCK_IMPL_CLOCK_IMPL_HPP
#define LIBP2P_CLOCK_IMPL_CLOCK_IMPL_HPP

#include "testutil/clock/clock.hpp"

namespace libp2p::clock {

  template <typename ClockType>
  class ClockImpl : public Clock<ClockType> {
   public:
    typename Clock<ClockType>::TimePoint now() const override;
  };

  // aliases for implementations
  using SteadyClockImpl = ClockImpl<std::chrono::steady_clock>;
  using SystemClockImpl = ClockImpl<std::chrono::system_clock>;

}  // namespace libp2p::clock

#endif  // LIBP2P_CLOCK_IMPL_CLOCK_IMPL_HPP
