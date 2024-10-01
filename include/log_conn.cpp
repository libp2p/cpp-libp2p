#include <boost/asio/ip/tcp.hpp>
#include <cstdio>
#include <libp2p/connection/secure_connection.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <log_conn.hpp>
#include <scale2.hpp>
#include <variant>

#define DEFINE(x) decltype(x) x

#define _IF_LOG_CONN if (not file)
#define IF_LOG_CONN _IF_LOG_CONN return
#define IF_LOG_CONN_ID _IF_LOG_CONN return next_event_id()
#define IF_LOG_CONN_CB \
  _IF_LOG_CONN return { 0, nullptr }
#define IF_LOG_CONN_LAYER           \
  _IF_LOG_CONN return {             \
    next_event_id(), { 0, nullptr } \
  }

namespace log_conn {
  using scale2::Compact32;
  using scale2::Compact64;

  struct Event {
    struct CbLost {
      Compact32 call_id;
    };
    struct CbMuch {
      Compact32 call_id;
    };
    struct Conn {
      struct Dns {
        struct Cb {
          Compact32 call_id;
          bool ok;
        };
      };
      struct Tcp {
        struct Dtor {};
        struct Close {};
        struct Dial {
          std::string addr;
        };
        struct Connect {
          struct Cb {
            Compact32 call_id;
            bool ok;
          };
          struct Timeout {};
        };
        struct Accept {
          std::string addr;
        };
      };
      struct Ssl {
        struct Dtor {};
        struct Close {};
        struct Cb {
          Compact32 call_id;
          bool ok;
        };
        Compact32 parent_id;
      };
      struct Ws {
        struct Dtor {};
        struct Close {};
        struct Cb {
          Compact32 call_id;
          bool ok;
        };
        Compact32 parent_id;
      };
      struct Noise {
        struct Dtor {};
        struct Close {};
        struct Mismatch {};
        struct Cb {
          Compact32 call_id;
          std::optional<std::string> peer;
        };
        Compact32 parent_id;
      };
      struct Yamux {
        struct Dtor {};
        struct Close {};
        Compact32 parent_id;
      };
      struct Stream {
        struct Dtor {};
        struct Close {};
        struct Reset {};
        struct Protocol {
          std::string proto;
        };
        Compact32 parent_id;
        bool out;
      };
      struct Read {
        Compact32 n;
        struct Cb {
          Compact32 call_id;
          std::optional<Bytes> buf;
        };
        struct CbSize {
          Compact32 call_id;
          std::optional<Compact32> n;
        };
      };
      struct Write {
        Bytes buf;
        struct Cb {
          Compact32 call_id;
          std::optional<Compact32> n;
        };
      };
      struct WriteSize {
        Compact32 n;
      };
      Compact32 conn_id;
      std::variant<Dns,
                   Dns::Cb,
                   Tcp::Dial,
                   Tcp::Dtor,
                   Tcp::Close,
                   Tcp::Connect,
                   Tcp::Connect::Cb,
                   Tcp::Connect::Timeout,
                   Tcp::Accept,
                   Ssl,
                   Ssl::Dtor,
                   Ssl::Close,
                   Ssl::Cb,
                   Ws,
                   Ws::Dtor,
                   Ws::Close,
                   Ws::Cb,
                   Noise,
                   Noise::Dtor,
                   Noise::Close,
                   Noise::Mismatch,
                   Noise::Cb,
                   Yamux,
                   Yamux::Dtor,
                   Yamux::Close,
                   Stream,
                   Stream::Dtor,
                   Stream::Close,
                   Stream::Reset,
                   Stream::Protocol,
                   Read,
                   Read::Cb,
                   Read::CbSize,
                   Write,
                   Write::Cb,
                   WriteSize>
          v;
    };
    Compact64 time;
    Compact32 event_id;
    std::variant<CbLost, CbMuch, Conn> v;
  };
  using Event_Conn = decltype(Event::Conn::v);

  DEFINE(with_bytes) = false;

  std::shared_ptr<FILE> file;
  void open(const std::string &path, size_t buf) {
    file = {fopen(path.c_str(), "ab"), fclose};
    setvbuf(file.get(), nullptr, _IOFBF, buf);
  }

  void push(const Event &event) {
    static Bytes len, out;
    len.resize(0);
    out.resize(0);
    scale2::encode(out, event);
    scale2::encode_compact({len}, out.size());
    qtils::append(len, out);
    if (fwrite(len.data(), len.size(), 1, file.get()) != 1) {
      throw std::runtime_error{"log_conn::push fwrite error"};
    }
  }

  uint64_t now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  Id next_event_id() {
    static Id x = 0;
    return x++;
  }

  void cb_lost(Id call_id) {
    IF_LOG_CONN;
    push({now(), next_event_id(), Event::CbLost{call_id}});
  }

  void cb_much(Id call_id) {
    IF_LOG_CONN;
    push({now(), next_event_id(), Event::CbMuch{call_id}});
  }
}  // namespace log_conn

#define OP_DTOR(x, y)                                 \
  void x##_dtor(Id conn_id) {                         \
    IF_LOG_CONN;                                      \
    push({                                            \
        now(),                                        \
        next_event_id(),                              \
        Event::Conn{conn_id, Event::Conn::y::Dtor{}}, \
    });                                               \
  }
#define OP_CLOSE(x, y)                                 \
  void x##_close(Id conn_id) {                         \
    IF_LOG_CONN;                                       \
    push({                                             \
        now(),                                         \
        next_event_id(),                               \
        Event::Conn{conn_id, Event::Conn::y::Close{}}, \
    });                                                \
  }
#define OP_LAYER(x, y)                                             \
  Layer x(Id parent_id) {                                          \
    IF_LOG_CONN_LAYER;                                             \
    auto id = next_event_id();                                     \
    push({now(), id, Event::Conn{id, Event::Conn::y{parent_id}}}); \
    return {                                                       \
        id,                                                        \
        {                                                          \
            id,                                                    \
            [=](bool ok) {                                         \
              push({                                               \
                  now(),                                           \
                  next_event_id(),                                 \
                  Event::Conn{id, Event::Conn::y::Cb{id, ok}},     \
              });                                                  \
            },                                                     \
        },                                                         \
    };                                                             \
  }

namespace log_conn::op {
  OP_DTOR(tcp, Tcp)
  OP_DTOR(ssl, Ssl)
  OP_DTOR(ws, Ws)
  OP_DTOR(noise, Noise)
  OP_DTOR(yamux, Yamux)
  OP_DTOR(stream, Stream)

  OP_CLOSE(tcp, Tcp)
  OP_CLOSE(ssl, Ssl)
  OP_CLOSE(ws, Ws)
  OP_CLOSE(noise, Noise)
  OP_CLOSE(yamux, Yamux)
  OP_CLOSE(stream, Stream)

  void stream_reset(Id conn_id) {
    IF_LOG_CONN;
    push({
        now(),
        next_event_id(),
        Event::Conn{conn_id, Event::Conn::Stream::Reset{}},
    });
  }

  Ok dns(Id conn_id) {
    IF_LOG_CONN_CB;
    auto id = next_event_id();
    push({now(), id, Event::Conn{conn_id, Event::Conn::Dns{}}});
    return {
        id,
        [=](bool ok) {
          push({
              now(),
              next_event_id(),
              Event::Conn{conn_id, Event::Conn::Dns::Cb{id, ok}},
          });
        },
    };
  }

  Id tcp_dial(const Multiaddress &addr, const PeerId &peer) {
    IF_LOG_CONN_ID;
    std::string s{addr.getStringAddress()};
    if (not addr.getPeerId()) {
      s += fmt::format("/p2p/{}", peer.toBase58());
    }
    auto id = next_event_id();
    push({
        now(),
        id,
        Event::Conn{id, Event::Conn::Tcp::Dial{s}},
    });
    return id;
  }

  Ok tcp_connect(Id conn_id) {
    IF_LOG_CONN_CB;
    auto id = next_event_id();
    push({now(), id, Event::Conn{conn_id, Event::Conn::Tcp::Connect{}}});
    return {
        id,
        [=](bool ok) {
          push({
              now(),
              next_event_id(),
              Event::Conn{conn_id, Event::Conn::Tcp::Connect::Cb{id, ok}},
          });
        },
    };
  }

  void tcp_connect_timeout(Id conn_id) {
    IF_LOG_CONN;
    push({
        now(),
        next_event_id(),
        Event::Conn{conn_id, Event::Conn::Tcp::Connect::Timeout{}},
    });
  }

  Id tcp_accept(const TcpSocket &socket) {
    IF_LOG_CONN_ID;
    auto id = next_event_id();
    auto _addr = socket.remote_endpoint();
    auto addr =
        reinterpret_cast<const boost::asio::ip::detail::endpoint &>(_addr)
            .to_string();
    push({
        now(),
        id,
        Event::Conn{id, Event::Conn::Tcp::Accept{addr}},
    });
    return id;
  }

  OP_LAYER(ssl, Ssl)
  OP_LAYER(ws, Ws)

  NoiseLayer noise(Id parent_id) {
    if (not file) {
      return {next_event_id(), {0, nullptr}};
    }
    auto id = next_event_id();
    push({now(), id, Event::Conn{id, Event::Conn::Noise{parent_id}}});
    return {
        id,
        {
            id,
            [=](NoiseCbArg r) {
              std::optional<std::string> peer;
              if (r) {
                peer = r.value()->remotePeer().value().toBase58();
              }
              push({
                  now(),
                  next_event_id(),
                  Event::Conn{id, Event::Conn::Noise::Cb{id, peer}},
              });
            },
        },
    };
  }

  void noise_mismatch(Id conn_id) {
    IF_LOG_CONN;
    push({
        now(),
        next_event_id(),
        Event::Conn{conn_id, Event::Conn::Noise::Mismatch{}},
    });
  }

  Id yamux(Id parent_id) {
    IF_LOG_CONN_ID;
    auto id = next_event_id();
    push({
        now(),
        id,
        Event::Conn{id, Event::Conn::Yamux{parent_id}},
    });
    return id;
  }

  Id stream(Id parent_id, bool out) {
    IF_LOG_CONN_ID;
    auto id = next_event_id();
    push({
        now(),
        id,
        Event::Conn{id, Event::Conn::Stream{parent_id, out}},
    });
    return id;
  }

  void stream_protocol(Id conn_id, std::string_view proto) {
    IF_LOG_CONN;
    push({
        now(),
        next_event_id(),
        Event::Conn{conn_id, Event::Conn::Stream::Protocol{std::string{proto}}},
    });
  }

  Io read(Id conn_id, BytesOut buf) {
    IF_LOG_CONN_CB;
    auto id = next_event_id();
    push({now(),
          id,
          Event::Conn{conn_id, Event::Conn::Read{(uint32_t)buf.size()}}});
    return {
        id,
        [=](std::optional<uint32_t> n) mutable {
          push({
              now(),
              next_event_id(),
              Event::Conn{
                  conn_id,
                  with_bytes ? Event_Conn{Event::Conn::Read::Cb{
                      id,
                      n ? std::make_optional(qtils::asVec(buf.first(*n)))
                        : std::nullopt,
                  }}
                             : Event_Conn{Event::Conn::Read::CbSize{id, n}},
              },
          });
        },
    };
  }

  Io write(Id conn_id, BytesIn buf) {
    IF_LOG_CONN_CB;
    auto id = next_event_id();
    push({
        now(),
        id,
        Event::Conn{
            conn_id,
            with_bytes ? Event_Conn{Event::Conn::Write{qtils::asVec(buf)}}
                       : Event_Conn{Event::Conn::WriteSize{buf.size()}},
        },
    });
    return {
        id,
        [=](std::optional<uint32_t> n) {
          push({
              now(),
              next_event_id(),
              Event::Conn{conn_id, Event::Conn::Write::Cb{id, n}},
          });
        },
    };
  }
}  // namespace log_conn::op

namespace log_conn::metrics {
  DEFINE(tcp_in);
  DEFINE(tcp_out);
  DEFINE(tcp_out_timeout);
  DEFINE(ssl);
  DEFINE(ws);
  DEFINE(noise);
}  // namespace log_conn::metrics
