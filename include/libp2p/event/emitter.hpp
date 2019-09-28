/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_EMITTER_HPP
#define LIBP2P_EMITTER_HPP

#include <functional>

#include <boost/signals2.hpp>
#include <libp2p/event/subscription.hpp>

namespace libp2p::event {

  namespace detail {

    /**
     * Base template prototype
     */
    template <int EventsAmount, typename... Events>
    class EmitterBase;

    /**
     * "Main" recursive template of the Emitter
     * @tparam EventsAmount - how much events there are totally
     * @tparam ThisEvent - event, for which this Emitter template is responsible
     * @tparam OtherEvents - zero or more events, which go after (\param
     * ThisEvent)
     */
    template <int EventsAmount, typename ThisEvent, typename... OtherEvents>
    class EmitterBase<EventsAmount, ThisEvent, OtherEvents...>
        : public EmitterBase<EventsAmount, OtherEvents...> {
      /// type of underlying signal
      template <typename EventType>
      using Signal = boost::signals2::signal<void(const EventType &)>;

      /// base of this template - in fact, the base's "ThisEvent" is the the
      /// first of this "OtherEvents"
      using Base = EmitterBase<EventsAmount, OtherEvents...>;

     protected:
      /**
       * Get an underlying signal, if (\tparam ExpectedEvent) is the same as
       * ThisEvent type
       * @return reference to signal
       */
      template <typename ExpectedEvent>
      std::enable_if_t<
          std::is_same_v<std::decay_t<ThisEvent>, std::decay_t<ExpectedEvent>>,
          Signal<ExpectedEvent>>
          &getSignal() {
        return signal_;
      }

      /**
       * SFINAE failure handler - continue the recursion, getting the base's
       * signal, if ThisEvent is not the same as (\tparam ExpectedEvent)
       * @return reference to signal
       */
      template <typename ExpectedEvent>
      std::enable_if_t<
          !std::is_same_v<std::decay_t<ThisEvent>, std::decay_t<ExpectedEvent>>,
          Signal<ExpectedEvent>>
          &getSignal() {
        return Base::template getSignal<ExpectedEvent>();
      }

     private:
      /// the underlying signal
      Signal<ThisEvent> signal_;
    };

    /**
     * End of template recursion
     */
    template <int EventsAmount>
    class EmitterBase<EventsAmount> {
     protected:
      virtual ~EmitterBase() = default;
    };

  }  // namespace detail

  /**
   * Emitter, allowing to subscribe to events and emit them
   * @tparam Events, for which this Emitter is responsible; should be empty
   * structs or structs with desired params of the events
   */
  template <typename... Events>
  class Emitter : public detail::EmitterBase<sizeof...(Events), Events...> {
    /// underlying recursive base of the emitter
    using Base = detail::EmitterBase<sizeof...(Events), Events...>;

   public:
    using emitter_t = Emitter<Events...>;

    /**
     * Subscribe to the specified event
     * @tparam Event, for which the subscription is to be registered
     * @param handler to be called, when a corresponding event is triggered
     * @return subscription to the event, which can be used, for example, to
     * unsubscribe
     */
    template <typename Event>
    Subscription on(const std::function<void(const Event &)> &handler) {
      return Subscription(Base::template getSignal<Event>().connect(handler));
    }

    /**
     * Trigger the specified event
     * @tparam Event, which is to be triggered
     * @param event - params (typically, a struct), with which the event is to
     * be triggered
     */
    template <typename Event>
    void emit(Event &&event) {
      Base::template getSignal<Event>()(std::forward<Event>(event));
    }
  };

}  // namespace libp2p::event

#endif  // LIBP2P_EMITTER_HPP
