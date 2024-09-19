#pragma once

#include <fmt/chrono.h>

inline auto time() {
  return fmt::format("{:%H:%M:%S}", fmt::localtime(time(nullptr)));
}
