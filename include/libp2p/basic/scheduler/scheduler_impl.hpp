/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <map>
#include <optional>
#include <variant>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/basic/scheduler/backend.hpp>

namespace libp2p::basic {

  /**
   * Scheduler implementation
   */
  class SchedulerImpl : public std::enable_shared_from_this<SchedulerImpl>,
                        public Scheduler,
                        public SchedulerBackendFeedback {
   public:
    /// Ctor, backend is injected
    SchedulerImpl(std::shared_ptr<SchedulerBackend> backend,
                  Scheduler::Config config);

    /// Returns current async
    std::chrono::milliseconds now() const override;

    /// Scheduler API impl
    Handle scheduleImpl(Callback &&cb,
                        std::chrono::milliseconds delay_from_now,
                        bool make_handle) override;

    /// Timer callback, called from SchedulerBackend
    void pulse() override;

   private:
    size_t callReady(Time now);

    /// Backend implementation
    std::shared_ptr<SchedulerBackend> backend_;

    /// Config
    const Scheduler::Config config_;

    struct CancelCb;
    using CancelCbPtr = std::shared_ptr<CancelCb>;
    using CancelOrCb = std::variant<CancelCbPtr, Callback>;
    using Callbacks = std::multimap<Time, CancelOrCb>;
    struct CancelCb {
      CancelCb(Callback &&cb) : cb{std::move(cb)} {}

      std::atomic_flag cancelled = false;
      std::optional<Callbacks::iterator> it;
      Callback cb;
    };
    Callbacks callbacks_;

    Time timer_;
  };
}  // namespace libp2p::basic
