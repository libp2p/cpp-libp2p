/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SCHEDULER_HPP
#define LIBP2P_SCHEDULER_HPP

#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>
#include <map>

namespace libp2p::protocol::kademlia {

  namespace scheduler {
    class Handle;

    // milliseconds in real case, whatever in test cases
    using Ticks = uint64_t;

    // counter is used for stable sort purpose
    // (i.e. callbacks scheduled earlier (but with the same time) will be invoked earlier)
    using Counter = uint64_t;

    using Ticket = std::pair<Ticks, Counter>;

    // Internal cancel scheduler ticket interface
    struct Cancellation {
      virtual ~Cancellation() = default;
      virtual void cancel(const Ticket &ticket) = 0;
    };

  } //namespace

  /// Async execution interface
  class Scheduler : public scheduler::Cancellation, private boost::noncopyable {
   public:
    using Ticks = scheduler::Ticks;
    using Counter = scheduler::Counter;
    using Ticket = scheduler::Ticket;
    using Callback = std::function<void()>;
    using Handle = std::unique_ptr<scheduler::Handle>;

    ~Scheduler() override;

    /// schedules delayed execution
    Handle schedule(Ticks delay, Callback cb);

    /// schedules immediate execution in the next reactor cycle,
    /// is effectively schedule(0, cb)
    Handle schedule(Callback cb);

   protected:
    Scheduler();

    // these 2 functions are to be implemented as per async backend (boost::asio or another)
    virtual Ticks now() = 0;
    virtual void scheduleImmediate() = 0;

    void pulse(bool immediate);

   private:
    // cancellation impl
    void cancel(const Ticket &ticket) override;

    scheduler::Handle *getHandle(bool immediate);

    std::map<Ticket, scheduler::Handle*> table_;
    Ticks last_tick_;
    Counter counter_;
  };

  namespace scheduler {

    // Lifetime-aware scheduler handle
    class Handle {
     public:
      ~Handle() {
        cancel();
      }

      void cancel() {
        if (scheduler_) {
          scheduler_->cancel(ticket_);
          scheduler_ = nullptr;
          cb_ = Scheduler::Callback();
        }
      }

     private:
      friend class libp2p::protocol::kademlia::Scheduler;

      Handle(Scheduler::Ticket ticket, scheduler::Cancellation *scheduler, Scheduler::Callback cb) :
        ticket_(std::move(ticket)), scheduler_(scheduler), cb_(std::move(cb)) {}

      void call() {
        scheduler_ = nullptr;
        cb_();
        // NB: after cb_(), *this may be invalid...
      }

      void done() {
        scheduler_ = nullptr;
      }

      Scheduler::Ticket ticket_;
      scheduler::Cancellation *scheduler_;
      Scheduler::Callback cb_;
    };
  } //namespace

} //namespace

#endif //LIBP2P_SCHEDULER_HPP
