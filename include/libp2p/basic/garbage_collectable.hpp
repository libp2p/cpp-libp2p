/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GARBAGE_COLLECTABLE_HPP
#define LIBP2P_GARBAGE_COLLECTABLE_HPP

namespace libp2p::basic {

  /**
   * @brief An abstraction over a data structure that can be garbage
   * collectable, i.e. garbage collector can cleanup this data
   * structure with given strategy.
   */
  struct GarbageCollectable {
    virtual ~GarbageCollectable() = default;

    /**
     * @brief Cleanup garbage once. Garbage collector calls this method
     * depending on the internal cleanup strategy.
     *
     * @note Caller must ensure that this method is called from the single
     * thread only.
     */
    virtual void collectGarbage() = 0;
  };

}  // namespace libp2p::basic

#endif  // LIBP2P_GARBAGE_COLLECTABLE_HPP
