/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace libp2p {
  /**
   * Allows using templates without void specialization.
   * `std::optional<void>`
   */
  class Void {};
}  // namespace libp2p
