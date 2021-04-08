/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_SCHEDULER_HPP
#define LIBP2P_PROTOCOL_SCHEDULER_HPP

#include <chrono>
#include <functional>
#include <map>
#include <memory>

#include <boost/noncopyable.hpp>

namespace libp2p::protocol {
  class Scheduler;

  namespace scheduler {
    // milliseconds in real case, whatever in test cases
    using Ticks = uint64_t;

    inline Ticks toTicks(std::chrono::seconds seconds) {
      namespace c = std::chrono;
      return c::duration_cast<c::milliseconds>(seconds).count();
    }

    // counter is used for stable sort purpose
    // (i.e. callbacks scheduled earlier (but with the same time) will be
    // invoked earlier)
    using Counter = uint64_t;

    using Ticket = std::pair<Ticks, Counter>;

    // Internal cancel scheduler ticket interface
    struct Cancellation : public std::enable_shared_from_this<Cancellation> {
      virtual ~Cancellation() = default;

      /// Cancels the ticket
      virtual void cancel(const Ticket &ticket) = 0;

      /// Reschedules the callback, returns a new ticket
      virtual Ticket reschedule(const Ticket &ticket, Ticks delay) = 0;
    };

    /// Lifetime-aware scheduler handle
    class Handle {
     public:
      Handle() = default;
      Handle(Handle &&) = default;
      Handle(const Handle &) = delete;
      Handle &operator=(const Handle &) = delete;

      ~Handle();

      /// Cancels this handle and takes ownership of r
      Handle &operator=(Handle &&r) noexcept;

      /// Detaches handle from feedback interface, won't cancel on out-of-scope
      void detach();

      void cancel();

      void reschedule(Ticks delay);

     private:
      friend class libp2p::protocol::Scheduler;

      Handle(Ticket ticket, std::weak_ptr<Cancellation> cancellation);

      Ticket ticket_;
      std::weak_ptr<Cancellation> cancellation_;
    };

  }  // namespace scheduler

  struct SchedulerConfig {
    scheduler::Ticks period_msec = 100;
  };

  /// Async execution interface
  class Scheduler : public scheduler::Cancellation, private boost::noncopyable {
   public:
    using Ticks = scheduler::Ticks;
    using Counter = scheduler::Counter;
    using Ticket = scheduler::Ticket;
    using Callback = std::function<void()>;
    using Handle = scheduler::Handle;

    ~Scheduler() override = default;

    /// schedules delayed execution
    Handle schedule(Ticks delay, Callback cb);

    /// schedules immediate execution in the next reactor cycle,
    /// is effectively schedule(0, cb)
    Handle schedule(Callback cb);

    /// to be implemented as per async backend
    virtual Ticks now() const = 0;

   protected:
    Scheduler();

    // to be implemented as per async backend (boost::asio or another)
    virtual void scheduleImmediate() = 0;

    void pulse(bool immediate);

   private:
    // cancellation impl
    void cancel(const Ticket &ticket) override;
    Ticket reschedule(const Ticket &ticket, Ticks delay) override;

    bool nextCallback(Ticks time);
    Ticket newTicket(Ticks delay, Callback cb);

    std::map<Ticket, Callback> table_;
    Counter counter_;

    // enable rescheduling from inside callback
    Counter counter_in_progress_ = 0;
    Callback cb_in_progress_;
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_SCHEDULER_HPP
