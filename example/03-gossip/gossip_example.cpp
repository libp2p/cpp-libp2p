/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>
#include <random>
#include <type_traits>

#include <spdlog/fmt/fmt.h>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>
#include <libp2p/protocol/gossip/impl/common.hpp>

#include "factory.hpp"

// wraps member fn via lambda
template <typename R, typename... Args, typename T>
auto bind_memfn(T *object, R (T::*fn)(Args...)) {
  return [object, fn](Args... args) {
    return (object->*fn)(std::forward<Args>(args)...);
  };
}
#define MEMFN_CALLBACK(M) \
  bind_memfn(this, &std::remove_pointer<decltype(this)>::type::M)

template <typename R, typename... Args, typename T, typename S>
auto bind_memfn_s(T *object, R (T::*fn)(const S &state, Args...),
                  const S &state) {
  return [object, fn, state = state](Args... args) {
    return (object->*fn)(state, std::forward<Args>(args)...);
  };
}
#define MEMFN_CALLBACK_S(M, S) \
  bind_memfn_s(this, &std::remove_pointer<decltype(this)>::type::M, S)

namespace {

  libp2p::common::Logger logger;

  void setupLoggers(bool log_debug) {
    static const char *kPattern = "%L %T.%e %v";

    logger = libp2p::common::createLogger("gossip-example");
    logger->set_pattern(kPattern);

    auto gossip_logger = libp2p::common::createLogger("gossip");
    gossip_logger->set_pattern(kPattern);
    if (log_debug) {
      gossip_logger->set_level(spdlog::level::debug);
    }

    auto debug_logger = libp2p::common::createLogger("debug");
    debug_logger->set_pattern(kPattern);
    debug_logger->set_level(spdlog::level::trace);
  }

}  // namespace

namespace libp2p::protocol::gossip::example {

  const std::string kAnnounceTopic = "+++";
  const std::string kDenounceTopic = "---";

  const Config &getConfig() {
    static Config config = ([] {
      Config c;
      c.D = 2;
      c.ideal_connections_num = 5;
      c.echo_forward_mode = true;
      return c;
    })();
    return config;
  }

  std::string toString(const ByteArray &buf) {
    // NOLINTNEXTLINE
    return std::string(reinterpret_cast<const char *>(buf.data()), buf.size());
  }

  multi::Multiaddress createAddress(size_t instance_no) {
    static const size_t kPortBase = 30000;
    auto res = multi::Multiaddress::create(
        fmt::format("/ip4/127.0.0.1/tcp/{}", kPortBase + instance_no));
    assert(res);
    return res.value();
  }

  class HostContext {
    size_t instance_no_ = 0;
    std::shared_ptr<Host> host_;
    std::shared_ptr<Gossip> gossip_;
    boost::optional<multi::Multiaddress> listen_address_;
    boost::optional<peer::PeerId> peer_id_;
    Subscription announce_sub_;
    std::unordered_map<TopicId, Subscription> subs_;
    int subs_counter_ = 0;

    // total receives by message emitted
    static std::map<std::string, size_t> receive_stats;

   public:
    static void printReceiveStats() {
      std::vector<std::string> strings{};
      strings.reserve(receive_stats.size());
      for (auto &[msg,count] : receive_stats) {
        strings.push_back(fmt::format("{} : {}", msg, count));
      }
      logger->info("Message receives:\n{}", fmt::join(strings, "\n"));
    }

    size_t instanceNo() const { return instance_no_; }

    peer::PeerId getPeerId() const {
      assert(peer_id_);
      return peer_id_.value();
    }

    boost::optional<multi::Multiaddress> getAddress() const {
      assert(listen_address_);
      return listen_address_;
    }

    HostContext(size_t i, const std::shared_ptr<Scheduler> &scheduler,
                const std::shared_ptr<boost::asio::io_context> &io) {
      instance_no_ = i;
      listen_address_ = createAddress(instance_no_);
      std::tie(host_, gossip_) =
          createHostAndGossip(getConfig(), scheduler, io);
      peer_id_ = host_->getId();
      announce_sub_ = gossip_->subscribe({kAnnounceTopic, kDenounceTopic},
                                         MEMFN_CALLBACK(onAnnounces));
      scheduler->schedule([this] { onStart(); }).detach();
    }

    void connectTo(peer::PeerId id,
                   boost::optional<multi::Multiaddress> address) {
      assert(gossip_);
      gossip_->addBootstrapPeer(std::move(id), std::move(address));
    }

    void subscribeTo(const TopicId &id) {
      assert(gossip_);
      if (subs_.count(id) == 0) {
        logger->info("({}) subscribes to {}", instance_no_, id);
        subs_[id] =
            gossip_->subscribe({id}, MEMFN_CALLBACK_S(onSubscription, id));
        ++subs_counter_;
      }
    }

    void unsubscribeFrom(const TopicId &id) {
      assert(gossip_);
      logger->info("({}) unsubscribes from {}", instance_no_, id);
      subs_.erase(id);
    }

    void onStart() {
      assert(host_);
      assert(listen_address_);
      assert(gossip_);

      auto listen_res = host_->listen(listen_address_.value());
      if (!listen_res) {
        logger->error("Host #{} cannot listen to multiaddress {}, {}",
                      instance_no_, listen_address_->getStringAddress(),
                      listen_res.error().message());
      }

      host_->start();
      gossip_->start();
    }

    void onSubscription(const TopicId &id, Gossip::SubscriptionData d) {
      if (!d) {
        logger->info("subscriptions stopped");
        subs_.clear();
      } else {
        auto res = peer::PeerId::fromBytes(d->from);
        std::string from = res ? res.value().toBase58().substr(46) : "???";
        std::string body = toString(d->data);
        logger->info("({}), subscr to {}, message from {}: {}, topics: [{}]",
                     instance_no_, id, from, body,
                     fmt::join(d->topics, ","));
        ++receive_stats[body];
      }
    }

    void onAnnounces(Gossip::SubscriptionData d) {
      if (!d) {
        logger->info("announces stopped");
      } else {
        if (d->topics[0] == kAnnounceTopic) {
          // we can subscribe from inside callback
          subscribeTo(toString(d->data));
        } else {
          // and unsubscribe
          unsubscribeFrom(toString(d->data));
        }
      }
    }

    void publish(const TopicSet &topics, const std::string &msg) {
      assert(gossip_);
      gossip_->publish(topics, fromString(msg));
    }
  };

  std::map<std::string, size_t> HostContext::receive_stats{};

  /// Emits topic activity in random manner
  class Emitter {
   public:
    Emitter(std::vector<HostContext> &hosts, Scheduler &scheduler)
        : hosts_(hosts), scheduler_(scheduler), eng_(rd_()) {
      create_next_ = scheduler_.schedule(MEMFN_CALLBACK(onNext));
    }

   private:
    struct FloodStats {
      TopicId topic;
      size_t sent = 0;
      size_t total = 0;
      Scheduler::Handle timer;
    };

    void onNext() {
      size_t sz = floods_.size();
      if (sz >= max_topics_) {
        create_next_.cancel();
      } else {
        createFlood(sz);
        create_next_.reschedule(chooseNextTopicInterval());
      }
    }

    void createFlood(size_t pos) {
      if (pos >= floods_.size()) {
        floods_.emplace_back();
        pos = floods_.size() - 1;
      }
      FloodStats &st = floods_[pos];
      st.topic = fmt::format("flood#{}", ++topic_counter_);
      st.sent = 0;
      st.total = chooseTopicTotalMessages();
      st.timer = scheduler_.schedule(chooseTimeInterval(),
                                     [this, pos] { onTimer(pos); });
      sendMessage(pos);
    }

    void onTimer(size_t pos) {
      assert(floods_.size() > pos);

      FloodStats &st = floods_[pos];
      if (st.sent >= st.total) {
        auto &host = chooseHost();

        // hosts will unsubscribe
        host.publish({kDenounceTopic}, st.topic);

        // create new topic and its lifetime
        createFlood(pos);
      } else {
        sendMessage(pos);
        st.sent++;
        st.timer.reschedule(chooseTimeInterval());
      }
    }

    void sendMessage(size_t pos) {
      assert(floods_.size() > pos);

      FloodStats &st = floods_[pos];
      if (st.sent == 0) {
        chooseHost().publish({kAnnounceTopic}, st.topic);
      }
      TopicSet topics{st.topic};
      if (msg_counter_ % 20 == 0) {
        auto additional_topics = rnd(0, 4);
        for (size_t i = 0; i < additional_topics; ++i) {
          topics.insert(chooseTopic());
        }
      }
      std::string msg = fmt::format("{:#06d}", ++msg_counter_);
      auto& h = chooseHost();
      logger->info("publishing {} on topics {} via host ({})", msg, fmt::join(topics, ","), h.instanceNo());
      chooseHost().publish(topics, msg);
    }

    const TopicId &chooseTopic() {
      assert(!floods_.empty());
      return floods_[rnd(0, floods_.size() - 1)].topic;
    }

    HostContext &chooseHost() {
      return hosts_[rnd(0, hosts_.size() - 1)];
    }

    size_t chooseTopicTotalMessages() {
      return rnd(13, 31);
    }

    size_t chooseTimeInterval() {
      return rnd(1000, 12345);
    }

    size_t chooseNextTopicInterval() {
      return rnd(3000, 27000);
    }

    using Distr = std::uniform_int_distribution<size_t>;

    size_t rnd(size_t lo, size_t hi) {
      return distr_(eng_, Distr::param_type(lo, hi));
    }

    std::vector<HostContext> &hosts_;
    Scheduler &scheduler_;
    std::random_device rd_;
    std::mt19937_64 eng_;
    Distr distr_;
    size_t topic_counter_ = 0;
    size_t msg_counter_ = 0;
    size_t max_topics_ = 66;
    std::vector<FloodStats> floods_;
    Scheduler::Handle create_next_;
  };

}  //  namespace libp2p::protocol::gossip::example

int main(int argc, char *argv[]) {
  namespace example = libp2p::protocol::gossip::example;
  try {
    size_t hosts_count = 5;
    bool log_debug = true;
    if (argc > 1)
      hosts_count = atoi(argv[1]);  // NOLINT
    if (argc > 2)
      log_debug = (atoi(argv[2]) != 0);  // NOLINT

    setupLoggers(log_debug);

    // create objects common to all hosts
    auto io = std::make_shared<boost::asio::io_context>();
    auto scheduler =
        std::make_shared<libp2p::protocol::AsioScheduler>(*io, 100);

    logger->info("Creating {} hosts", hosts_count);
    std::vector<example::HostContext> hosts;
    hosts.reserve(hosts_count);
    for (size_t i = 0; i < hosts_count; ++i) {
      hosts.emplace_back(i, scheduler, io);
    }

    // bootstrap peers, let them know each other, they would connect
    // in a random manner as per config parameters
    logger->info("Bootstrapping peers addresses");
    for (size_t i = 0; i < hosts_count; ++i) {
      const auto &h = hosts[i];
      for (size_t j = 0; j < hosts_count; ++j) {
        // if (libp2p::protocol::gossip::less(hosts[j].getPeerId(),
        // h.getPeerId())) {
        if (i != j) {
          hosts[j].connectTo(h.getPeerId(), h.getAddress());
        }
      }
    }

    logger->info("Starting");
    auto emitter = std::make_unique<example::Emitter>(hosts, *scheduler);

    // run until signal is caught
    boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
    signals.async_wait(
        [&io](const boost::system::error_code &, int) { io->stop(); });
    io->run();

    logger->info("Stopping");
    emitter.reset();
    hosts.clear();

    logger->info("Stopped");

    example::HostContext::printReceiveStats();

  } catch (const std::exception &e) {
    if (logger) {
      logger->error("Exception: {}", e.what());
    }
  }
}
