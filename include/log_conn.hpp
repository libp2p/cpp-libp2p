#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <qtils/bytes.hpp>
#include <string>

#define LOG_CONN_DTOR(x) log_conn::op::x##_dtor(conn_id_.value())
#define LOG_CONN_CLOSE(x) log_conn::op::x##_close(conn_id_.value())
#define LOG_CONN_READ auto op = log_conn::op::read(conn_id_.value(), out)
#define LOG_CONN_WRITE auto op = log_conn::op::write(conn_id_.value(), in)

namespace boost::asio {
  class any_io_executor;
  template <typename, typename>
  class basic_stream_socket;
}  // namespace boost::asio

namespace boost::asio::ip {
  class tcp;
}  // namespace boost::asio::ip

namespace libp2p::connection {
  class SecureConnection;
}  // namespace libp2p::connection

namespace libp2p::multi {
  class Multiaddress;
}  // namespace libp2p::multi

namespace libp2p::peer {
  class PeerId;
}  // namespace libp2p::peer

namespace libp2p {
  using multi::Multiaddress;
  using peer::PeerId;
}  // namespace libp2p

namespace log_conn {
  using libp2p::Multiaddress;
  using libp2p::PeerId;
  using qtils::Bytes;
  using qtils::BytesIn;
  using qtils::BytesOut;
  using TcpSocket =
      boost::asio::basic_stream_socket<boost::asio::ip::tcp,
                                       boost::asio::any_io_executor>;

  using Id = uint32_t;

  extern bool with_bytes;

  void open(const std::string &path, size_t buf = 64 << 10);

  void cb_lost(Id call_id);
  void cb_much(Id call_id);

  template <typename T>
  struct Cb {
    using F = std::function<void(T)>;
    Cb(Id call_id, auto f)
        : call_id_{call_id}, f_{std::make_shared<F>(std::move(f))} {}
    ~Cb() {
      if (not f_) {
        return;
      }
      if (f_.unique() and *f_) {
        cb_lost(call_id_);
      }
    }
    void operator()(T t) const {
      if (not f_) {
        return;
      }
      if (*f_) {
        auto f = std::exchange(*f_, {});
        f(std::forward<T>(t));
      } else {
        cb_much(call_id_);
      }
    }
    void operator()(auto &&ec, auto &&r) const {
      if (not f_) {
        return;
      }
      (*this)(ec ? std::nullopt
                 : std::make_optional(std::forward<decltype(r)>(r)));
    }
    auto asio(auto cb) const {
      return [op{*this}, cb{std::move(cb)}](auto &&ec, auto &&r) mutable {
        op(ec, r);
        cb(std::forward<decltype(ec)>(ec), std::forward<decltype(r)>(r));
      };
    }
    void wrap(auto &cb) const {
      cb = [op{*this}, cb{std::move(cb)}](auto &&r) mutable {
        if constexpr (std::is_same_v<T, bool>) {
          op((bool)r);
        } else {
          op(r ? std::make_optional(r.value()) : std::nullopt);
        }
        cb(std::forward<decltype(r)>(r));
      };
    }
    Id call_id_;
    std::shared_ptr<F> f_;
  };
}  // namespace log_conn

namespace log_conn::op {
  using Ok = Cb<bool>;
  using Layer = std::pair<Id, Ok>;
  void tcp_dtor(Id conn_id);
  void ssl_dtor(Id conn_id);
  void ws_dtor(Id conn_id);
  void noise_dtor(Id conn_id);
  void yamux_dtor(Id conn_id);
  void stream_dtor(Id conn_id);
  void tcp_close(Id conn_id);
  void ssl_close(Id conn_id);
  void ws_close(Id conn_id);
  void noise_close(Id conn_id);
  void yamux_close(Id conn_id);
  void stream_close(Id conn_id);
  void stream_reset(Id conn_id);
  Ok dns(Id conn_id);
  Id tcp_dial(const Multiaddress &addr, const PeerId &peer);
  Ok tcp_connect(Id conn_id);
  void tcp_connect_timeout(Id conn_id);
  Id tcp_accept(const TcpSocket &socket);
  Layer ssl(Id parent_id);
  Layer ws(Id parent_id);
  using NoiseCbArg = const outcome::result<
      std::shared_ptr<libp2p::connection::SecureConnection>> &;
  using NoiseLayer = std::pair<Id, Cb<NoiseCbArg>>;
  NoiseLayer noise(Id parent_id);
  void noise_mismatch(Id conn_id);
  Id yamux(Id parent_id);
  Id stream(Id parent_id, bool out);
  void stream_protocol(Id conn_id, std::string_view proto);
  using Io = Cb<std::optional<uint32_t>>;
  Io read(Id conn_id, BytesOut buf);
  Io write(Id conn_id, BytesIn buf);
}  // namespace log_conn::op

namespace log_conn::metrics {
  using U = std::atomic_size_t;
  struct OkErr {
    U ok, err;
    void operator()(bool r) {
      ++(r ? ok : err);
    }
  };
  template <typename T>
  struct InOut {
    T in, out;
    T &operator[](bool out_) {
      return out_ ? out : in;
    }
  };
  extern U tcp_in;
  extern OkErr tcp_out;
  extern U tcp_out_timeout;
  extern InOut<OkErr> ssl, ws, noise;
}  // namespace log_conn::metrics
