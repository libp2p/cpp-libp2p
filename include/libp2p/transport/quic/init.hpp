#pragma once

#include <lsquic.h>
#include <stdexcept>

namespace libp2p::transport {
  inline void lsquicInit() {
    static auto _ = [] {
      if (lsquic_global_init(LSQUIC_GLOBAL_CLIENT | LSQUIC_GLOBAL_SERVER)
          != 0) {
        throw std::logic_error{"lsquic_global_init"};
      }
      return 0;
    }();
  }
}  // namespace libp2p::transport
