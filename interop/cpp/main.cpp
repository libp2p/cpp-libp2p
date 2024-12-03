#include <deque>

#include <fmt/format.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/injector/kademlia_injector.hpp>
#include <libp2p/protocol/gossip/di.hpp>
#include <libp2p/protocol/identify.hpp>
#include <libp2p/soralog.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>
#include <qtils/append.hpp>
#include <qtils/bytestr.hpp>
#include <qtils/unhex.hpp>

#include "interop.pb.h"

template <typename T>
using SP = std::shared_ptr<T>;

using libp2p::asioBuffer;
using libp2p::Host;
using libp2p::Multiaddress;
using libp2p::PeerId;
using libp2p::StreamAndProtocol;
using libp2p::connection::CapableConnection;
using libp2p::connection::Stream;
using libp2p::peer::PeerInfo;
using libp2p::protocol::Identify;
using libp2p::protocol::gossip::Gossip;
using libp2p::protocol::gossip::GossipDi;
using libp2p::protocol::kademlia::Kademlia;
using libp2p::transport::detail::asTcp;
using qtils::asVec;
using qtils::byte2str;
using qtils::str2byte;
using Ec = boost::system::error_code;
using _Io = boost::asio::io_context;
using Io = SP<_Io>;
using _Sock = boost::asio::ip::tcp::socket;
using Sock = SP<_Sock>;
using Buf = SP<qtils::Bytes>;
using GossipCfg = libp2p::protocol::gossip::Config;

struct SecureAdaptorProxy : libp2p::security::SecurityAdaptor {
  libp2p::peer::ProtocolName getProtocolId() const override {
    return impl->getProtocolId();
  }
  void secureInbound(SP<libp2p::connection::LayerConnection> inbound,
                     SecConnCallbackFunc cb) override {
    return impl->secureInbound(inbound, cb);
  }
  void secureOutbound(SP<libp2p::connection::LayerConnection> outbound,
                      const PeerId &p,
                      SecConnCallbackFunc cb) override {
    return impl->secureOutbound(outbound, p, cb);
  }
  SP<libp2p::security::SecurityAdaptor> impl;
};

auto multiaddress(std::string_view s) {
  return Multiaddress::create(s).value();
}
auto multiaddress(qtils::BytesIn a) {
  return Multiaddress::create(a).value();
}
auto asTcp2(const Multiaddress &addr) {
  return asTcp(addr).value().first.asTcp().value();
}

struct EncodeVar {
  qtils::BytesN<9> buf;
  uint8_t size = 0;
  EncodeVar(uint64_t x) {
    while (x) {
      auto y = x & 0x7f;
      x >>= 7;
      buf.at(size++) = y | (x ? 0x80 : 0);
    }
  }
  auto bytes() const {
    return std::span{buf}.first(size);
  }
};

#define SELF \
  this, self { shared_from_this() }

void _readFrame(Sock sock, Buf buf, uint8_t bits, uint64_t var, auto f) {
  if (bits >= 32) {
    buf->resize(0);
    return f(false);
  }
  buf->resize(1);
  boost::asio::async_read(
      *sock,
      asioBuffer(*buf),
      [sock, buf, bits, var, f](Ec ec, size_t) mutable {
        if (ec) {
          buf->resize(0);
          return f(false);
        }
        auto &x = (*buf)[0];
        var |= (x & 0x7f) << bits;
        bits += 7;
        if (x & 0x80) {
          return _readFrame(sock, buf, bits, var, f);
        }
        buf->resize(var);
        boost::asio::async_read(
            *sock, asioBuffer(*buf), [buf, f](Ec ec, size_t) mutable {
              if (ec) {
                buf->resize(0);
                return f(false);
              }
              f(true);
            });
      });
}
void readFrame(Sock sock, Buf buf, auto f) {
  _readFrame(sock, buf, 0, 0, f);
}
template <typename T>
  requires(requires(const T &pb) { pb.ByteSizeLong(); })
void appendFrame(Buf buf, const T &pb) {
  auto n = pb.ByteSizeLong();
  EncodeVar v{n};
  auto m = v.size + n;
  buf->reserve(buf->size() + m);
  qtils::append(*buf, v.bytes());
  buf->resize(buf->size() + n);
  if (not pb.SerializeToArray(buf->data() + buf->size() - n, n)) {
    throw std::runtime_error{"pb.encode"};
  }
}
void write(Sock sock, Buf buf, auto f) {
  boost::asio::async_write(
      *sock, asioBuffer(*buf), [sock, buf, f](Ec ec, size_t) { f(not ec); });
}
template <typename T>
  requires(requires(const T &pb) { pb.ByteSizeLong(); })
void write(Sock sock, Buf buf, const T &pb, auto f) {
  buf->resize(0);
  appendFrame(buf, pb);
  write(sock, buf, f);
}
void writeError(Sock sock, Buf buf, std::error_code ec) {
  pb::Response res;
  res.set_type(pb::Response_Type_ERROR);
  *res.mutable_error()->mutable_msg() = fmt::to_string(ec);
  write(sock, buf, res, [](bool) {});
}

void pbSet(auto *pb, const PeerInfo &info) {
  *pb->mutable_id() = byte2str(info.id.toVector());
  for (auto &addr : info.addresses) {
    *pb->add_addrs() = byte2str(addr.getBytesAddress());
  }
}
auto pbOk() {
  pb::Response res;
  res.set_type(pb::Response_Type_OK);
  return res;
}
auto pbPeer(const auto &pb) {
  return PeerId::fromBytes(qtils::str2byte(pb.peer())).value();
}
auto pbProto(const auto &pb) {
  return libp2p::StreamProtocols{pb.proto().begin(), pb.proto().end()};
}
auto pbInfo(const StreamAndProtocol &r) {
  pb::StreamInfo pb;
  *pb.mutable_proto() = r.protocol;
  *pb.mutable_peer() = byte2str(r.stream->remotePeerId().value().toVector());
  *pb.mutable_addr() =
      byte2str(r.stream->remoteMultiaddr().value().getBytesAddress());
  return pb;
}

void pipe(SP<Stream> r, Sock w, Buf buf) {
  r->readSome(*buf, buf->size(), [r, w, buf](outcome::result<size_t> _n) {
    if (not _n) {
      w->shutdown(_Sock::shutdown_send);
      return;
    }
    auto &n = _n.value();
    boost::asio::async_write(
        *w, asioBuffer(std::span{*buf}.first(n)), [r, w, buf](Ec ec, size_t) {
          if (ec) {
            return;
          }
          pipe(r, w, buf);
        });
  });
}
void pipe(Sock r, SP<Stream> w, Buf buf) {
  r->async_read_some(asioBuffer(*buf), [r, w, buf](Ec ec, size_t n) {
    if (not n) {
      w->close([](outcome::result<void>) {});
      return;
    }
    libp2p::write(
        w, std::span{*buf}.first(n), [r, w, buf](outcome::result<void> _n) {
          if (not _n) {
            return;
          }
          pipe(r, w, buf);
        });
  });
}
void pipe2(SP<Stream> stream, Sock sock, Buf buf) {
  buf->resize(64 << 10);
  pipe(stream, sock, buf);
  pipe(sock, stream, std::make_shared<qtils::Bytes>(buf->size()));
}

struct PubSub_Sub : std::enable_shared_from_this<PubSub_Sub> {
  PubSub_Sub(SP<Gossip> pubsub, Sock sock, Buf buf, std::string topic)
      : pubsub_{pubsub}, sock_{sock}, buf_{buf}, topic_{topic} {}
  void ctor_shared() {
    sub_ = pubsub_->subscribe(
        {topic_},
        [SELF](
            boost::optional<const libp2p::protocol::gossip::Gossip::Message &>
                m) {
          if (not m) {
            return;
          }
          queue_.emplace_back(m->data);
          _write();
        });
    write(sock_, buf_, pbOk(), [SELF](bool ok) {
      if (not ok) {
        return;
      }
      writing_ = false;
      _write();
    });
  }
  void _write() {
    if (writing_) {
      return;
    }
    if (queue_.empty()) {
      return;
    }
    pb::PSMessage req;
    *req.add_topicids() = topic_;
    *req.mutable_data() = byte2str(queue_.front());
    queue_.pop_front();
    writing_ = true;
    write(sock_, buf_, req, [SELF](bool ok) {
      if (not ok) {
        return;
      }
      writing_ = false;
      _write();
    });
  }
  SP<Gossip> pubsub_;
  Sock sock_;
  Buf buf_;
  std::string topic_;
  libp2p::protocol::Subscription sub_;
  std::deque<qtils::Bytes> queue_;
  bool writing_ = true;
};

struct Control {
  Multiaddress v;
  Control(int, ...) = delete;
  Control(Multiaddress v) : v{v} {}
};
class Daemon : public std::enable_shared_from_this<Daemon> {
 public:
  Daemon(Io io,
         SP<Host> host,
         SP<Kademlia> kad,
         const GossipDi &pubsub,
         Control control)
      : io_{io},
        host_{host},
        kad_{kad},
        pubsub_{pubsub.impl},
        control_{control.v},
        acceptor_{*io_} {}
  void ctor_shared() {
    acceptor_.open(boost::asio::ip::tcp::v4());
    acceptor_.bind(asTcp2(control_));
    acceptor_.listen();
    accept();
  }

 private:
  void accept() {
    acceptor_.async_accept([SELF](Ec ec, _Sock &&sock) {
      if (ec) {
        return;
      }
      read(std::make_shared<_Sock>(std::move(sock)));
      accept();
    });
  }
  void read(Sock sock) {
    auto buf = std::make_shared<qtils::Bytes>();
    readFrame(sock, buf, [SELF, sock, buf](bool ok) {
      if (not ok) {
        return;
      }
      pb::Request req;
      if (not req.ParseFromArray(buf->data(), buf->size())) {
        throw std::runtime_error{"Request.decode"};
      }
      switch (req.type()) {
        case pb::Request_Type_IDENTIFY: {
          auto res = pbOk();
          pbSet(res.mutable_identify(), host_->getPeerInfo());
          write(sock, buf, res, [](bool) {});
          break;
        }
        case pb::Request_Type_CONNECT: {
          PeerInfo info{pbPeer(req.connect()), {}};
          for (auto &addr : req.connect().addrs()) {
            info.addresses.emplace_back(multiaddress(qtils::str2byte(addr)));
          }
          host_->connect(info,
                         [sock, buf](outcome::result<SP<CapableConnection>> r) {
                           if (not r) {
                             return writeError(sock, buf, r.error());
                           }
                           write(sock, buf, pbOk(), [](bool) {});
                         });
          break;
        }
        case pb::Request_Type_STREAM_OPEN: {
          host_->newStream(pbPeer(req.streamopen()),
                           pbProto(req.streamopen()),
                           [sock, buf](outcome::result<StreamAndProtocol> _r) {
                             if (not _r) {
                               return writeError(sock, buf, _r.error());
                             }
                             auto &r = _r.value();
                             auto res = pbOk();
                             *res.mutable_streaminfo() = pbInfo(r);
                             write(sock, buf, res, [r, sock, buf](bool ok) {
                               if (not ok) {
                                 return;
                               }
                               pipe2(r.stream, sock, buf);
                             });
                           });
          break;
        }
        case pb::Request_Type_STREAM_HANDLER: {
          auto addr =
              asTcp2(multiaddress(qtils::str2byte(req.streamhandler().addr())));
          host_->setProtocolHandler(
              pbProto(req.streamhandler()), [SELF, addr](StreamAndProtocol r) {
                auto sock = std::make_shared<_Sock>(*io_);
                sock->async_connect(addr, [r, sock](Ec ec) {
                  if (ec) {
                    return;
                  }
                  auto buf = std::make_shared<qtils::Bytes>();
                  write(sock, buf, pbInfo(r), [r, sock, buf](bool ok) {
                    if (not ok) {
                      return;
                    }
                    pipe2(r.stream, sock, buf);
                  });
                });
              });
          write(sock, buf, pbOk(), [](bool) {});
          break;
        }
        case pb::Request_Type_DHT: {
          switch (req.dht().type()) {
            case pb::DHTRequest_Type_FIND_PEER: {
              std::ignore = kad_->findPeer(
                  pbPeer(req.dht()),
                  [sock, buf](outcome::result<PeerInfo> _r,
                              std::vector<PeerId>) {
                    if (not _r) {
                      return writeError(sock, buf, _r.error());
                    }
                    auto &r = _r.value();
                    auto res = pbOk();
                    res.mutable_dht()->set_type(pb::DHTResponse_Type_VALUE);
                    pbSet(res.mutable_dht()->mutable_peer(), r);
                    write(sock, buf, res, [](bool) {});
                  });
              break;
            }
            case pb::DHTRequest_Type_FIND_PEERS_CONNECTED_TO_PEER: {
              fmt::println(
                  "TODO(cpp): DHTRequest_Type_FIND_PEERS_CONNECTED_TO_PEER");
              break;
            }
            case pb::DHTRequest_Type_FIND_PROVIDERS: {
              std::ignore = kad_->findProviders(
                  asVec(str2byte(req.dht().cid())),
                  req.dht().count(),
                  [sock, buf](outcome::result<std::vector<PeerInfo>> _r) {
                    if (not _r) {
                      return writeError(sock, buf, _r.error());
                    }
                    auto &r = _r.value();
                    buf->resize(0);
                    auto begin = pbOk();
                    begin.mutable_dht()->set_type(pb::DHTResponse_Type_BEGIN);
                    appendFrame(buf, begin);
                    pb::DHTResponse item;
                    item.set_type(pb::DHTResponse_Type_VALUE);
                    for (auto &x : r) {
                      pbSet(item.mutable_peer(), x);
                      appendFrame(buf, item);
                    }
                    pb::DHTResponse end;
                    end.set_type(pb::DHTResponse_Type_END);
                    appendFrame(buf, end);
                    write(sock, buf, [](bool) {});
                  });
              break;
            }
            case pb::DHTRequest_Type_GET_CLOSEST_PEERS: {
              fmt::println("TODO(cpp): DHTRequest_Type_GET_CLOSEST_PEERS");
              break;
            }
            case pb::DHTRequest_Type_GET_PUBLIC_KEY: {
              fmt::println("TODO(cpp): DHTRequest_Type_GET_PUBLIC_KEY");
              break;
            }
            case pb::DHTRequest_Type_GET_VALUE: {
              std::ignore = kad_->getValue(
                  asVec(str2byte(req.dht().key())),
                  [sock, buf](outcome::result<qtils::Bytes> _r) {
                    if (not _r) {
                      return writeError(sock, buf, _r.error());
                    }
                    auto &r = _r.value();
                    auto res = pbOk();
                    res.mutable_dht()->set_type(pb::DHTResponse_Type_VALUE);
                    *res.mutable_dht()->mutable_value() = byte2str(r);
                    write(sock, buf, res, [](bool) {});
                  });
              break;
            }
            case pb::DHTRequest_Type_SEARCH_VALUE: {
              fmt::println("TODO(cpp): DHTRequest_Type_SEARCH_VALUE");
              break;
            }
            case pb::DHTRequest_Type_PUT_VALUE: {
              std::ignore = kad_->putValue(asVec(str2byte(req.dht().key())),
                                           asVec(str2byte(req.dht().value())));
              write(sock, buf, pbOk(), [](bool) {});
              break;
            }
            case pb::DHTRequest_Type_PROVIDE: {
              std::ignore =
                  kad_->provide(asVec(str2byte(req.dht().cid())), true);
              write(sock, buf, pbOk(), [](bool) {});
              break;
            }
            default:
              break;
          }
          break;
        }
        case pb::Request_Type_LIST_PEERS: {
          auto res = pbOk();
          for (auto &c :
               host_->getNetwork().getConnectionManager().getConnections()) {
            auto peer_id = c->remotePeer().value();
            pbSet(res.add_peers(),
                  PeerInfo{peer_id, {c->remoteMultiaddr().value()}});
          }
          write(sock, buf, res, [](bool) {});
          break;
        }
        case pb::Request_Type_CONNMANAGER: {
          fmt::println("TODO(cpp): CONNMANAGER");
          break;
        }
        case pb::Request_Type_DISCONNECT: {
          fmt::println("TODO(cpp): DISCONNECT");
          break;
        }
        case pb::Request_Type_PUBSUB: {
          switch (req.pubsub().type()) {
            case pb::PSRequest_Type_GET_TOPICS: {
              fmt::println("TODO(cpp): PSRequest_Type_GET_TOPICS");
              break;
            }
            case pb::PSRequest_Type_LIST_PEERS: {
              auto &topic = req.pubsub().topic();
              auto peers = pubsub_->subscribers(topic);
              auto res = pbOk();
              *res.mutable_pubsub()->add_topics() = topic;
              for (auto &peer : peers) {
                *res.mutable_pubsub()->add_peerids() =
                    byte2str(peer.toVector());
              }
              write(sock, buf, res, [](bool) {});
              break;
            }
            case pb::PSRequest_Type_PUBLISH: {
              pubsub_->publish(req.pubsub().topic(),
                               asVec(str2byte(req.pubsub().data())));
              write(sock, buf, pbOk(), [](bool) {});
              break;
            }
            case pb::PSRequest_Type_SUBSCRIBE: {
              std::make_shared<PubSub_Sub>(
                  pubsub_, sock, buf, req.pubsub().topic())
                  ->ctor_shared();
              break;
            }
            default:
              break;
          }
          break;
        }
        case pb::Request_Type_PEERSTORE: {
          fmt::println("TODO(cpp): PEERSTORE");
          break;
        }
        default:
          break;
      }
    });
  }

  Io io_;
  SP<Host> host_;
  SP<Kademlia> kad_;
  SP<Gossip> pubsub_;
  Multiaddress control_;
  boost::asio::ip::tcp::acceptor acceptor_;
};

int main(int argn, char **argv) {
  setvbuf(stdout, nullptr, _IOLBF, 0);
  setvbuf(stderr, nullptr, _IOLBF, 0);
  libp2p_soralog();

  auto args = std::span{argv, (size_t)argn}.subspan(1);
  auto arg_control = multiaddress(args[0]);
  auto arg_key = qtils::unhex<qtils::BytesN<64>>(args[1]).value();
  auto arg_encryption = args[2];
  auto arg_listen = args.subspan(3);

  GossipCfg gossip_cfg;
  gossip_cfg.sign_messages = true;
  auto di = libp2p::injector::makeHostInjector(
      boost::di::bind<Control>().to(std::make_shared<Control>(arg_control)),
      libp2p::injector::useKeyPair({
          {libp2p::crypto::Key::Type::Ed25519,
           qtils::asVec(std::span{arg_key}.subspan(32))},
          {libp2p::crypto::Key::Type::Ed25519,
           qtils::asVec(std::span{arg_key}.first(32))},
      }),
      libp2p::injector::useSecurityAdaptors<SecureAdaptorProxy>(),
      boost::di::bind<GossipCfg>().to(std::move(gossip_cfg)),
      libp2p::injector::makeKademliaInjector());
  auto &encryption = di.create<SecureAdaptorProxy &>();
  std::map<std::string, std::function<void()>> encryption_map{
      {"plaintext",
       [&] { encryption.impl = di.create<SP<libp2p::security::Plaintext>>(); }},
      {"noise",
       [&] { encryption.impl = di.create<SP<libp2p::security::Noise>>(); }},
  };
  encryption_map.at(arg_encryption)();
  auto io = di.create<Io>();
  auto host = di.create<SP<Host>>();
  auto daemon = di.create<SP<Daemon>>();
  auto id = di.create<SP<Identify>>();
  auto kad = di.create<SP<Kademlia>>();
  auto pubsub = di.create<SP<GossipDi>>()->impl;
  daemon->ctor_shared();
  host->start();
  id->start();
  kad->start();
  pubsub->start();
  id->onIdentifyReceived([host, kad, pubsub](const PeerId &peer_id) {
    auto info = host->getPeerRepository().getPeerInfo(peer_id);
    kad->addPeer(info, false, true);
    for (auto &addr : info.addresses) {
      pubsub->addBootstrapPeer(info.id, addr);
    }
  });

  for (auto &arg : arg_listen) {
    host->listen(multiaddress(arg)).value();
  }

  io->run();
}
