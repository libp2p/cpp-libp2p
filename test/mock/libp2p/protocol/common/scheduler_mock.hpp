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

  /*
   TODO(artem) : gmock vs.noexcept

   struct SchedulerMock : public basic::Scheduler {
    ~SchedulerMock() override = default;

    MOCK_METHOD0(now, std::chrono::milliseconds());
    MOCK_METHOD3(scheduleImpl,
                 Handle(Callback &&, std::chrono::milliseconds, bool));

    MOCK_METHOD1(cancel, void(Handle::Ticket));
    MOCK_METHOD2(reschedule,
                 outcome::result<Handle::Ticket>(Handle::Ticket,
                                                 std::chrono::milliseconds));
  };
   */

}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_SCHEDULERMOCK
