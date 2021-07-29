/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_SCHEDULER_IMPL_HPP
#define LIBP2P_BASIC_SCHEDULER_IMPL_HPP

#include <map>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>
#include <gsl/span>

#include <libp2p/basic/scheduler.hpp>

namespace libp2p::basic {

  /**
   * Scheduler implementation
   */
  class SchedulerImpl : public Scheduler {
   public:
    /// Ctor, backend is injected
    SchedulerImpl(std::shared_ptr<SchedulerBackend> backend,
                  Scheduler::Config config);

   private:
    using Ticket = Scheduler::Handle::Ticket;

    /// Returns current async
    std::chrono::milliseconds now() const noexcept override;

    /// Scheduler API impl
    Handle scheduleImpl(Callback &&cb, std::chrono::milliseconds delay_from_now,
                        bool make_handle) noexcept override;

    /// Cancels the ticket, called from Handle
    void cancel(Handle::Ticket ticket) noexcept override;

    /// Reschedules the ticket, called from Handle
    outcome::result<Handle::Ticket> reschedule(
        Handle::Ticket ticket,
        std::chrono::milliseconds delay_from_now) noexcept override;

    /// Timer callback, called from SchedulerBackend
    void pulse(std::chrono::milliseconds current_clock) noexcept override;

    struct Item {
      uint64_t seq = 0;
      Callback cb;

      Item(uint64_t _seq, Callback _cb) : seq(_seq), cb(std::move(_cb)) {}
    };

    /// Container for simplest case of callbacks
    class DeferredCallbacksWithoutCancel {
     public:
      /// Saves item for later execution
      void push(uint64_t seq, Callback cb);

      /// Returns if no items
      bool empty() const;

      /// Moves items to be processed now
      void get(std::vector<Item> &items_to_process);

     private:
      /// Items deferred to the next IO cycle
      std::vector<Item> items_;
    };

    /// Container for cancellable (i.e. with handles) deferred callbacks
    class DeferredCallbacksWithCancel {
     public:
      /// Saves item for later execution
      void push(uint64_t seq, Callback cb);

      /// Returns if no items
      bool empty() const;

      /// Moves items to be processed in this cycle
      void startProcessing(std::vector<uint64_t> &items_to_process);

      /// Executes callback, if found by seq
      void execute(uint64_t seq);

      /// Called after execution cycle
      void endProcessing();

      /// Cancels callback and returns cancelled callback as it might be
      /// rescheduled to timed callbacks
      Callback cancel(uint64_t seq);

     private:
      /// Item seqs to be processed in the next IO loop cycle
      std::vector<uint64_t> items_;

      // TODO(artem): use flat hash map for performance

      /// Callback maps stored separately, to ease cancelling, which is allowed
      /// from inside callbacks themselves
      using Table = std::unordered_map<uint64_t, Callback>;

      /// Table being populated, will be deferred to next cycle
      Table deferred_;

      /// Table being processed at the moment
      Table being_processed_;

      /// Seq number being processed at the moment
      uint64_t seq_in_process_ = 0;

      /// Callback being executed at the moment
      Callback cb_in_process_;
    };

    /// Controls deferred callbacks and their timer
    class DeferredCallbacks {
     public:
      /// Ctor
      explicit DeferredCallbacks(SchedulerBackend &backend);

      /// Adds item and reschedules timer if needed
      void push(uint64_t seq, Callback cb, bool cancellable,
                std::weak_ptr<SchedulerBackendFeedback> sch);

      /// Timer callback, owner arg helps control lifetime
      void onTimer(std::shared_ptr<Scheduler> owner);

      /// Cancels callback and returns cancelled callback as it might be
      /// rescheduled to timed callbacks
      Callback cancel(uint64_t seq);

     private:
      /// Processes non-cancellable current items, in absense of cancellable
      /// ones. Called from onTimer()
      static void processNonCancellableItems(
          gsl::span<Item> items, const std::shared_ptr<Scheduler> &owner);

      /// Processes cancellable current items, in absense of non-cancellable
      /// ones. Called from onTimer()
      void processCancellableItems(gsl::span<uint64_t> items,
                                   const std::shared_ptr<Scheduler> &owner);

      /// Processes both cancellable and non-cancellable items in proper order
      /// of seq numbers. Called from onTimer()
      void processBothSources(const std::shared_ptr<Scheduler> &owner);

      /// Reschedules callback from the backend
      void rescheduleTimer(std::weak_ptr<SchedulerBackendFeedback> sch);

      /// Backend reference to control timer
      SchedulerBackend &backend_;

      /// Non-cancellable callbacks container (performance optimized as the
      /// simplest ans most used case)
      DeferredCallbacksWithoutCancel non_cancellable_;

      /// Cancellable callbacks container
      DeferredCallbacksWithCancel cancellable_;

      /// True if timer is set at the moment
      bool timer_is_set_ = false;

      /// Non-cancellable items currently in process, helps avoid extra dynamic
      /// allocations
      std::vector<Item> current_nc_items_;

      /// Cancellable items currently in process, helps avoid extra dynamic
      /// allocations
      std::vector<uint64_t> current_c_items_;
    };

    /// Controls delayed callbacks
    class TimedCallbacks {
     public:
      /// Ctor
      TimedCallbacks(SchedulerBackend &backend,
                     std::chrono::milliseconds timer_threshold);

      /// Adds item and reschedules timer if needed
      void push(std::chrono::milliseconds abs_time, uint64_t seq, Callback cb,
                std::weak_ptr<SchedulerBackendFeedback> sch);

      /// Timer callback, owner arg helps control lifetime
      void onTimer(std::chrono::milliseconds clock,
                   std::shared_ptr<Scheduler> owner);

      /// Cancels an item and reschedules timer if needed
      void cancel(const Ticket &ticket,
                  std::weak_ptr<SchedulerBackendFeedback> sch);

      /// Rechedules timed ticket, may return errors, see Scheduler API
      outcome::result<Ticket> rescheduleTicket(
          const Ticket &ticket, std::chrono::milliseconds abs_time,
          uint64_t new_seq);

     private:
      /// Reschedules timer
      void rescheduleTimer(std::weak_ptr<SchedulerBackendFeedback> sch);

      /// Backend reference to control timer
      SchedulerBackend &backend_;

      /// Timer threshold in ticks, helps to avoid often timer switches
      /// up to reasonable resolution
      std::chrono::milliseconds timer_threshold_;

      /// Items sorted
      std::map<Ticket, Callback> items_;

      /// Non-zero expiration if timer is set
      std::chrono::milliseconds current_timer_ = kZeroTime;

      /// Seq number being processed at the moment
      uint64_t seq_in_process_ = 0;

      /// Helper needed to allow for rescheduling from inside current callback
      boost::optional<Ticket> current_callback_rescheduled_;
    };

    /// Backend implementation
    std::shared_ptr<SchedulerBackend> backend_;

    /// Config
    const Scheduler::Config config_;

    /// Seq number incremented on every new task
    uint64_t seq_number_ = 0;

    /// Deferred callbacks
    DeferredCallbacks deferred_callbacks_;

    /// Timed callbacks
    TimedCallbacks timed_callbacks_;
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_SCHEDULER_IMPL_HPP
