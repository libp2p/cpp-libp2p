/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/scheduler.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {

  struct SchedulerMock : public Scheduler {
    MOCK_METHOD(std::chrono::milliseconds, now, (), (const, override));

    MOCK_METHOD(Cancel,
                scheduleImpl,
                (Callback &&, std::chrono::milliseconds, bool),
                (override));
  };

}  // namespace libp2p::basic
