/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/outcome.hpp>

namespace libp2p {
  using CbOutcomeVoid = std::function<void(outcome::result<void>)>;
}  // namespace libp2p
