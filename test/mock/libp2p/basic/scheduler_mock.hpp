/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_SCHEDULERMOCK
#define LIBP2P_BASIC_SCHEDULERMOCK

#include <libp2p/basic/scheduler.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {

  struct SchedulerMock : public Scheduler {
    /*
     * Overridance of noexcept methods done with wrapping of MOCK_METHODS.
     * The new syntax of google tests v1.10 allows to pass the qualifiers
     * https://google.github.io/googletest/gmock_for_dummies.html#how-to-define-it
     * Thus the best way to get rid of this ugly code is to update google tests.
     */

    MOCK_METHOD1(pulseMockCall, void(std::chrono::milliseconds));
    void pulse(std::chrono::milliseconds current_clock) noexcept override {
      pulseMockCall(current_clock);
    }

    MOCK_CONST_METHOD0(nowMockCall, std::chrono::milliseconds());
    std::chrono::milliseconds now() const noexcept override {
      return nowMockCall();
    }

    MOCK_METHOD3(scheduleImplMockCall,
                 Handle(Callback &, std::chrono::milliseconds, bool));
    MOCK_METHOD1(cancelMockCall, void(Handle::Ticket));
    MOCK_METHOD2(rescheduleMockCall,
                 outcome::result<Handle::Ticket>(Handle::Ticket,
                                                 std::chrono::milliseconds));

   protected:
    Handle scheduleImpl(Callback &&cb, std::chrono::milliseconds delay_from_now,
                        bool make_handle) noexcept override {
      return scheduleImplMockCall(cb, delay_from_now, make_handle);
    }

    void cancel(Handle::Ticket ticket) noexcept override {
      cancelMockCall(ticket);
    }

    outcome::result<Handle::Ticket> reschedule(
        Handle::Ticket ticket,
        std::chrono::milliseconds delay_from_now) noexcept override {
      return rescheduleMockCall(ticket, delay_from_now);
    }
  };

}  // namespace libp2p::basic

#endif  // LIBP2P_BASIC_SCHEDULERMOCK
