/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/async/clock.hpp"

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
