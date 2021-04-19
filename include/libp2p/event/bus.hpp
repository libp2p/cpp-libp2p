/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BUS_HPP
#define LIBP2P_BUS_HPP

#include <map>
#include <memory>
#include <typeindex>

#include <boost/asio.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/signals2.hpp>

#include <libp2p/log/logger.hpp>

/**
 * Most of the implementation here is taken from
 * https://github.com/EOSIO/appbase
 */
namespace libp2p::event {
  using erased_channel_ptr = std::unique_ptr<void, void (*)(void *)>;

  /**
   * A basic DispatchPolicy that will catch and drop any exceptions thrown
   * during the dispatch of messages on a channel
   */
  struct drop_exceptions {
    drop_exceptions() = default;
    using result_type = void;

    template <typename InputIterator>
    result_type operator()(InputIterator first, InputIterator last) {
      while (first != last) {
        try {
          *first;
        } catch (const std::exception &e) {
          // drop
          log::createLogger("Bus")->error(
              "Exception in signal handler, ignored, what={}", e.what());
        } catch (...) {
          // drop
          log::createLogger("Bus")->error(
              "Exception in signal handler, ignored");
        }
        ++first;
      }
    }
  };

  /**
   * Type that represents an active subscription to a channel allowing
   * for ownership via RAII and also explicit unsubscribe actions
   */
  class Handle {
   public:
    ~Handle() {
      unsubscribe();
    }

    /**
     * Explicitly unsubscribe from channel before the lifetime
     * of this object expires
     */
    void unsubscribe() noexcept {
      if (handle_.connected()) {
        try {
          handle_.disconnect();
        } catch (...) {
          log::createLogger("Bus")->error("disconnect handle caused exception");
        }
      }
    }

    // This handle can be constructed and moved
    Handle() = default;

    /// Cancels existing connection
    Handle &operator=(Handle &&rhs) noexcept {
      unsubscribe();
      handle_ = std::move(rhs.handle_);

      assert(!rhs.handle_.connected());  // move was correct?

      return *this;
    }

    // no way they can be noexcept because of none-noexcept
    // boost::signal2::connection move ctor
    Handle(Handle &&) = default;  // NOLINT

    // dont allow copying since this protects the resource
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;

   private:
    using handle_type = boost::signals2::scoped_connection;
    handle_type handle_;

    /**
     * Construct a handle from an internal representation of a handle
     * In this case a boost::signals2::connection
     *
     * @param _handle - the boost::signals2::connection to wrap
     */
    explicit Handle(handle_type &&handle) : handle_(std::move(handle)) {}

    template <typename D, typename DP>
    friend class Channel;
  };

  /**
   * Channel, which can emit events and allows to subscribe to them
   */
  template <typename Data, typename DispatchPolicy>
  class Channel {
   public:
    /**
     * Subscribe to data on a channel
     * @tparam Callback the type of the callback (functor|lambda)
     * @param cb the callback
     * @return handle to the subscription
     */
    template <typename Callback>
    Handle subscribe(Callback &&cb) {
      return Handle(signal_.connect(std::forward<Callback>(cb)));
    }

    /**
     * Publish an event to the channel
     * @param data to be published
     */
    void publish(const Data &data) {
      if (hasSubscribers()) {
        signal_(data);
      }
    }

    /**
     * Returns whether or not there are subscribers
     */
    bool hasSubscribers() {
      auto connections = signal_.num_slots();
      return connections > 0;
    }

   private:
    Channel() = default;
    virtual ~Channel() = default;

    /**
     * Proper deleter for type-erased channel
     * @note no type checking is performed at this level
     * @param erased_channel_ptr
     */
    static void deleter(void *erased_channel_ptr) {
      auto ptr =
          reinterpret_cast<  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
              Channel *>(erased_channel_ptr);
      delete ptr;  // NOLINT(cppcoreguidelines-owning-memory)
    }

    /**
     * Get the channel back from an erased pointer
     * @param ptr - the type-erased channel pointer
     * @return - the type safe channel pointer
     */
    static Channel *get_channel(erased_channel_ptr &ptr) {
      return reinterpret_cast<  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
          Channel *>(ptr.get());
    }

    /**
     * Construct a unique_ptr for the type erased method poiner
     */
    static erased_channel_ptr make_unique() {
      return erased_channel_ptr(new Channel(), &deleter);
    }

    boost::signals2::signal<void(const Data &), DispatchPolicy> signal_;

    friend class Bus;
  };

  /**
   * Declaration of the channel, which is to be used in Bus
   * @tparam Tag - API specific discriminator used to distinguish between
   * otherwise identical data types
   * @tparam Data - the type of the Data the channel carries
   * @tparam DispatchPolicy - The dispatch policy to use for this channel
   * (defaults to @ref drop_exceptions)
   */
  template <typename Tag, typename Data,
            typename DispatchPolicy = drop_exceptions>
  struct channel_decl {
    using channel_type = Channel<Data, DispatchPolicy>;
    using tag_type = Tag;
  };

  template <typename... Ts>
  std::true_type is_channel_decl_impl(const channel_decl<Ts...> *);

  std::false_type is_channel_decl_impl(...);

  template <typename T>
  using is_channel_decl = decltype(is_channel_decl_impl(std::declval<T *>()));

  /**
   * Event bus, containing channels and providing a convenient access to them
   */
  class Bus : private boost::noncopyable {
   public:
    /**
     * Fetch a reference to the channel declared by the passed in type. This
     * will construct the channel on first access
     * @tparam ChannelDecl - @ref libp2p::event::channel_decl
     * @return reference to the channel described by the declaration
     */
    template <typename ChannelDecl>
    auto getChannel()
        -> std::enable_if_t<is_channel_decl<ChannelDecl>::value,
                            typename ChannelDecl::channel_type &> {
      using channel_type = typename ChannelDecl::channel_type;
      auto key = std::type_index(typeid(ChannelDecl));
      auto itr = channels_.find(key);
      if (itr != channels_.end()) {
        return *channel_type::get_channel(itr->second);
      }
      channels_.emplace(std::make_pair(key, channel_type::make_unique()));
      return *channel_type::get_channel(channels_.at(key));
    }

   private:
    std::map<std::type_index, erased_channel_ptr> channels_;
  };
}  // namespace libp2p::event

#endif  // LIBP2P_BUS_HPP
