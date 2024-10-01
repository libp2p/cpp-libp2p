#pragma once

#include <qtils/append.hpp>
#include <qtils/bytestr.hpp>

#define _SCALE2_TIE(...)          \
  auto tie() const {              \
    auto &[__VA_ARGS__] = *this;  \
    return std::tie(__VA_ARGS__); \
  }
#define SCALE2_TIE_1 _SCALE2_TIE(v0)
#define SCALE2_TIE_2 _SCALE2_TIE(v0, v1)
#define SCALE2_TIE_3 _SCALE2_TIE(v0, v1, v2)

namespace scale2 {
  struct Out {
    qtils::Bytes &v;
  };
  void encode(qtils::Bytes &out, const auto &...v) {
    encode(Out{out}, v...);
  }
  void encode(Out out, const auto &v0, const auto &v1, const auto &...v) {
    encode(out, v0);
    encode(out, v1, v...);
  }
  template <typename T>
  void encode(Out out, const T &)
    requires std::is_empty_v<T>
  {}
  void encode(Out out, const auto &v)
    requires(requires { v.tie(); })
  {
    std::apply([&](const auto &...v) { encode(out, v...); }, v.tie());
  }
  void _encode_int(Out out, const auto &v, size_t n) {
    qtils::append(out.v, qtils::BytesIn{(const uint8_t *)&v, n});
  }
  void encode(Out out, const uint8_t &v) {
    _encode_int(out, v, 1);
  }
  void encode(Out out, const uint16_t &v) {
    _encode_int(out, v, 2);
  }
  void encode(Out out, const uint32_t &v) {
    _encode_int(out, v, 4);
  }
  void encode(Out out, const uint64_t &v) {
    _encode_int(out, v, 8);
  }
  void encode_compact(Out out, uint64_t v) {
    auto bits = 64 - __builtin_clzll(v);
    if (bits <= 8 - 2) {
      return encode(out, (uint8_t)(v << 2));
    }
    if (bits <= 16 - 2) {
      return encode(out, (uint16_t)((v << 2) | 1));
    }
    if (bits <= 32 - 2) {
      return encode(out, (uint32_t)((v << 2) | 2));
    }
    if (bits <= 52) {
      auto bytes = (bits + 7) / 8;
      encode(out, (uint8_t)(((bytes - 4) << 2) | 3));
      _encode_int(out, v, bytes);
      return;
    }
    throw std::runtime_error{"encode_compact"};
  }
  struct Compact32 {
    Compact32(uint32_t v) : v{v} {}
    uint32_t v;
  };
  void encode(Out out, const Compact32 &v) {
    encode_compact(out, v.v);
  }
  struct Compact64 {
    Compact64(uint64_t v) : v{v} {}
    uint64_t v;
  };
  void encode(Out out, const Compact64 &v) {
    encode_compact(out, v.v);
  }
  void encode(Out out, const bool &v) {
    encode(out, (uint8_t)(v ? 1 : 0));
  }
  template <typename T>
  void encode(Out out, const std::optional<T> &v) {
    if (v) {
      encode(out, (uint8_t)1, *v);
    } else {
      encode(out, (uint8_t)0);
    }
  }
  void encode(Out out, const qtils::Bytes &v) {
    encode_compact(out, v.size());
    qtils::append(out.v, v);
  }
  void encode(Out out, const std::string_view &v) {
    encode_compact(out, v.size());
    qtils::append(out.v, qtils::str2byte(v));
  }
  template <typename... T>
  void encode(Out out, const std::variant<T...> &v) {
    encode(out, (uint8_t)v.index());
    visit([&](const auto &v) { encode(out, v); }, v);
  }
}  // namespace scale2
