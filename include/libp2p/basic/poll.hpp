/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <optional>
#include <qtils/outcome.hpp>

/**
 * Const state can be moved to arguments to save heap memory.
 *   Foo{mut1, const1, const2}.poll()
 *   Foo{mut1}.poll(const1, const2)
 *
 * Argument references aren't captured so don't expire.
 *   Foo{ref1}.poll()
 *   Foo{}.poll(ref1)
 */

#define POLL_READY(x)             \
  ({                              \
    auto poll_try = (x);          \
    if (not poll_try.isReady()) { \
      return PollPending{};       \
    }                             \
    poll_try.ready();             \
  })

// #define POLL_TRY_BREAK(x)         \
//   ({                              \
//     auto poll_try = (x);          \
//     if (not poll_try.isReady()) { \
//       break;                      \
//     }                             \
//     poll_try.ready();             \
//   })

// #define POLL_OK_TRY(x)                         \
//   ({                                           \
//     auto ok_poll_try = (x);                    \
//     if (not ok_poll_try.isReady()) {           \
//       return PollPending{};                    \
//     }                                          \
//     if (not ok_poll_try.ready().has_value()) { \
//       return ::libp2p::PollOutcomeError{};          \
//     }                                          \
//     *ok_poll_try.ready();                      \
//   })

namespace libp2p {
  /**
   * Poll operation result is not ready.
   *
   * https://doc.rust-lang.org/std/task/enum.Poll.html
   */
  class PollPending {};

  /**
   * Poll operation result can be ready or pending.
   *
   * https://doc.rust-lang.org/std/task/enum.Poll.html
   */
  template <typename T>
  class Poll {
   public:
    Poll(const PollPending &) : ready_{std::nullopt} {}
    Poll(T ready) : ready_{std::move(ready)} {}

    bool isReady() const {
      return ready_.has_value();
    }
    auto &ready() const {
      return ready_.value();
    }
    auto &ready() {
      return ready_.value();
    }

   private:
    std::optional<T> ready_;
  };
  template <typename T>
  class Poll<outcome::result<T>> {
   public:
    Poll(const PollPending &) : ready_{std::nullopt} {}
    // Poll(outcome::result<T> ready) : ready_{std::move(ready)} {}
    // Poll(outcome::result<T> &&ready) : ready_{std::move(ready)} {}
    // Poll(T ready_value) : ready_{std::move(ready_value)} {}
    // Poll(T ready_value) : ready_{std::move(ready_value)} {}
    Poll(std::error_code ready_error) : ready_{std::move(ready_error)} {}

    bool isReady() const {
      return ready_.has_value();
    }
    auto &ready() const {
      return ready_.value();
    }
    auto &ready() {
      return ready_.value();
    }

   private:
    std::optional<outcome::result<T>> ready_;
  };

  /**
   * Called when poll can be called again.
   *
   * https://doc.rust-lang.org/std/task/struct.Waker.html
   */
  using PollWaker = std::function<void()>;

  template <typename T>
  using PollOutcome = Poll<outcome::result<T>>;

  // class PollOutcomeError {
  //  public:
  //   template <typename T>
  //   operator PollOutcome<T>() const {
  //     return std::nullopt;
  //   }
  // };
}  // namespace libp2p
