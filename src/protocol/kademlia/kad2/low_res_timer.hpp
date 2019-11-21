#ifndef LIBP2P_LOW_RES_TIMER_HPP
#define LIBP2P_LOW_RES_TIMER_HPP

namespace libp2p::kad2 {

  class LowResTimer {
   public:
    using Ticks = uint64_t;
    using Ticket = std::pair<Ticks, uint64_t>;
    using Callback = std::function<void()>;

    class HandleImpl {
     public:
      ~HandleImpl() {
        cancel();
      }

      void cancel() {
        if (timer_) {
          timer_->cancel(ticket_);
          timer_ = nullptr;
          cb_ = Callback();
        }
      }

     private:
      friend class LowResTimer;

      HandleImpl(Ticket ticket, LowResTimer *timer, Callback cb) :
        ticket_(std::move(ticket)), timer_(timer), cb_(std::move(cb)) {}

      void onTimer() {
        timer_ = nullptr;
        cb_();
        cb_ = Callback();
      }

      void done() {
        timer_ = nullptr;
      }

      Ticket ticket_;
      LowResTimer *timer_;
      Callback cb_;
    };

    using Handle = std::unique_ptr<HandleImpl>;

    virtual ~LowResTimer() {
      for (auto &p : table_) {
        p.second->done();
      }
    }

    Handle create(uint64_t interval, Callback cb) {
      Ticket ticket(now() + interval, ++counter_);
      Handle h(new HandleImpl(ticket, this, std::move(cb)));
      table_[ticket] = h.get();
      return h;
    }

    void cancel(const Ticket &ticket) {
      table_.erase(ticket);
    }

   protected:
    LowResTimer() : last_tick_(0), counter_(0) {}

    virtual Ticks now() = 0;

    void pulse() {
      last_tick_ = now();
      for (;;) {
        HandleImpl *h = getHandle();
        if (!h) {
          break;
        }
        h->onTimer();
      }
    }

   private:
    HandleImpl *getHandle() {
      if (table_.empty()) {
        return nullptr;
      }
      auto it = table_.begin();
      if (it->first.first > last_tick_) {
        return nullptr;
      }
      HandleImpl *h = it->second;
      table_.erase(it);
      return h;
    }

    std::map<Ticket, HandleImpl *> table_;
    uint64_t last_tick_;
    uint64_t counter_;
  };

  class LowResTimerAsioImpl : public LowResTimer {
   public:
    LowResTimerAsioImpl(boost::asio::io_context &io, Ticks interval) :
      LowResTimer{},
      timer_(io, boost::posix_time::milliseconds(interval)),
      interval_(interval),
      started_(boost::posix_time::microsec_clock::local_time()),
      cb_([this](const boost::system::error_code &) { onTimer(); }) {
      timer_.async_wait(cb_);
    }

   private:
    Ticks now() override {
      boost::posix_time::ptime t(boost::posix_time::microsec_clock::local_time());
      return (t - started_).total_milliseconds();
    }

    void onTimer() {
      pulse();
      timer_.expires_at(timer_.expires_at()
                        + boost::posix_time::milliseconds(interval_));
      timer_.async_wait(cb_);
    }

    boost::asio::deadline_timer timer_;
    Ticks interval_;
    boost::posix_time::ptime started_;
    std::function<void(const boost::system::error_code &)> cb_;
  };

} //namespace

#endif //LIBP2P_LOW_RES_TIMER_HPP
