/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/macro/common.hpp>

#define _IF_ERROR_CB_RETURN(tmp, r) \
  ({                                \
    auto &&_r = r;                  \
    if (not _r.has_value()) {       \
      return cb(_r.error());        \
    }                               \
    _r.value();                     \
  })
#define IF_ERROR_CB_RETURN(r) _IF_ERROR_CB_RETURN(QTILS_UNIQUE_NAME(tmp), r)
