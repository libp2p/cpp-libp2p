/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio/ssl/context.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/security/tls/ssl_context.hpp>
#include <libp2p/security/tls/tls_details.hpp>
#include <qtils/bytes.hpp>

namespace libp2p::security {
  constexpr qtils::BytesN<1 + 6> kAlpn{6, 'l', 'i', 'b', 'p', '2', 'p'};

  static int alpnSelect(SSL *ssl,
                        const unsigned char **out,
                        unsigned char *outlen,
                        const unsigned char *in,
                        unsigned int inlen,
                        void *) {
    int r = SSL_select_next_proto(const_cast<unsigned char **>(out),
                                  outlen,
                                  in,
                                  inlen,
                                  kAlpn.data(),
                                  kAlpn.size());
    return r == OPENSSL_NPN_NEGOTIATED ? SSL_TLSEXT_ERR_OK
                                       : SSL_TLSEXT_ERR_ALERT_FATAL;
  }

  SslContext::SslContext(
      const peer::IdentityManager &idmgr,
      const crypto::marshaller::KeyMarshaller &key_marshaller) {
    using boost::asio::ssl::context;
    auto r = tls_details::makeCertificate(idmgr.getKeyPair(), key_marshaller);
    auto make = [&] {
      auto ctx = std::make_shared<context>(context::tlsv13);
      ctx->set_options(context::no_compression | context::no_sslv2
                       | context::no_sslv3 | context::no_tlsv1_1
                       | context::no_tlsv1_2);
      ctx->set_verify_mode(boost::asio::ssl::verify_peer
                           | boost::asio::ssl::verify_fail_if_no_peer_cert
                           | boost::asio::ssl::verify_client_once);
      ctx->set_verify_callback(&tls_details::verifyCallback);
      ctx->use_certificate(asioBuffer(r.certificate), context::asn1);
      ctx->use_private_key(asioBuffer(r.private_key), context::asn1);
      static FILE *keylog = [] {
        FILE *f = nullptr;
        if (auto path = getenv("SSLKEYLOGFILE")) {
          f = fopen(path, "a");
          setvbuf(f, nullptr, _IOLBF, 0);
        }
        return f;
      }();
      if (keylog) {
        SSL_CTX_set_keylog_callback(
            ctx->native_handle(), +[](const SSL *, const char *line) {
              fputs(line, keylog);
              fputc('\n', keylog);
            });
      }
      return ctx;
    };
    tls = make();
    quic = make();
    SSL_CTX_set_alpn_protos(quic->native_handle(), kAlpn.data(), kAlpn.size());
    SSL_CTX_set_alpn_select_cb(quic->native_handle(), alpnSelect, nullptr);
  }
}  // namespace libp2p::security
