#pragma once

#include <boost/variant.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <qtils/append.hpp>
#include <qtils/bytes.hpp>
#include <qtils/unhex.hpp>
#include <scale/scale.hpp>

namespace scale {
  template <typename T>
  concept Tie = requires(T &t) { t.tie(); };

  template <Tie T>
  auto &operator>>(ScaleDecoderStream &s, T &v) {
    return s >> v.tie();
  }
  template <Tie T>
  auto &operator<<(ScaleEncoderStream &s, const T &v) {
    return s << const_cast<T &>(v).tie();
  }

  using ScalePeerId = qtils::BytesN<38>;
  inline auto &operator>>(ScaleDecoderStream &s, libp2p::PeerId &v) {
    ScalePeerId as;
    s >> as;
    v = libp2p::PeerId::fromBytes(as).value();
    return s;
  }
  inline auto &operator<<(ScaleEncoderStream &s, const libp2p::PeerId &v) {
    ScalePeerId as;
    auto bytes = v.toVector();
    if (bytes.size() != as.size()) {
      throw std::runtime_error{"scale PeerId"};
    }
    memcpy(as.data(), bytes.data(), as.size());
    return s << as;
  }
}  // namespace scale

namespace audi {
  using libp2p::PeerId;

  inline libp2p::crypto::KeyPair replay_peer() {
    libp2p::crypto::ed25519::Keypair k;
    qtils::unhex(
        k.private_key,
        "f8dfdb0f1103d9fb2905204ac32529d5f148761c4321b2865b0a40e15be75f57")
        .value();
    libp2p::crypto::ed25519::Ed25519ProviderImpl ed;
    k.public_key = ed.derive(k.private_key).value();
    return k;
  }

  using Key32 = qtils::BytesN<32>;
  struct ReplayEventPeer {
    PeerId peer;
    uint32_t clz;
    auto tie() {
      return std::tie(peer, clz);
    }
  };
  struct ReplayEventQuery {
    Key32 key;
    uint32_t clz;
    std::vector<PeerId> peers;
    auto tie() {
      return std::tie(key, clz, peers);
    }
  };
  struct ReplayEventRequest {
    Key32 key;
    PeerId peer;
    auto tie() {
      return std::tie(key, peer);
    }
  };
  struct ReplayEventResponse {
    Key32 key;
    PeerId peer;
    std::vector<PeerId> peers;
    std::optional<qtils::Bytes> value;
    auto tie() {
      return std::tie(key, peer, peers, value);
    }
  };
  using ReplayEvent = boost::variant<ReplayEventPeer,
                                     ReplayEventQuery,
                                     ReplayEventRequest,
                                     ReplayEventResponse>;
  struct Replay {
    std::vector<std::pair<uint64_t, ReplayEvent>> events;
  };

  inline Key32 key32(qtils::BytesIn k) {
    Key32 r;
    if (k.size() != r.size()) {
      throw std::runtime_error{"key32"};
    }
    memcpy(r.data(), k.data(), r.size());
    return r;
  }

  inline uint64_t now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  struct ReplayWriter {
    FILE *file_;
    ReplayWriter(const std::string &path) : file_{fopen(path.c_str(), "w")} {
      if (not file_) {
        throw std::runtime_error{path};
      }
      setvbuf(file_, nullptr, _IONBF, 0);
    }
    void write(ReplayEvent event) {
      auto tmp = scale::encode(now(), event).value();
      auto buf = scale::encode<uint32_t>(tmp.size()).value();
      qtils::append(buf, tmp);

      if (fwrite(buf.data(), buf.size(), 1, file_) != 1) {
        throw std::runtime_error{"write"};
      }
    }
    void peer(const PeerId &peer, uint32_t clz) {
      write(ReplayEventPeer{peer, clz});
    }
    void query(qtils::BytesIn key,
               uint32_t clz,
               const std::vector<PeerId> &peers) {
      write(ReplayEventQuery{key32(key), clz, peers});
    }
    void request(qtils::BytesIn key, const PeerId &peer) {
      write(ReplayEventRequest{key32(key), peer});
    }
    void response(qtils::BytesIn key,
                  const PeerId &peer,
                  const std::vector<PeerId> &peers,
                  const std::optional<qtils::Bytes> &value) {
      write(ReplayEventResponse{key32(key), peer, peers, value});
    }
  };
  inline auto &replay_writer() {
    static std::optional<ReplayWriter> x;
    return x;
  }
  inline void replay_writer_env() {
    if (auto path = getenv("AUDI_WRITE")) {
      replay_writer() = ReplayWriter{path};
    }
  }
}  // namespace audi
