/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_SCHEDULERMOCK
#define LIBP2P_PROTOCOL_SCHEDULERMOCK

#include <gmock/gmock.h>
#include "libp2p/protocol/common/scheduler.hpp"

namespace libp2p::protocol {

  struct SchedulerMock : public Scheduler {
    ~SchedulerMock() override = default;

    MOCK_CONST_METHOD0(now, Ticks());
    MOCK_METHOD0(scheduleImmediate, void());

    MOCK_METHOD1(cancel, void(const Ticket &));
    MOCK_METHOD2(reschedule, Ticket(const Ticket &, Ticks));
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_SCHEDULERMOCK
