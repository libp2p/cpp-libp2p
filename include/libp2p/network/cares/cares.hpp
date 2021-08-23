/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_NETWORK_CARES_CARES_HPP
#define LIBP2P_INCLUDE_LIBP2P_NETWORK_CARES_CARES_HPP

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <ares.h>
#include <arpa/nameser.h>
#include <sys/select.h>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::network::c_ares {

  /**
   *
   * Only one instance is allowed to exist.
   * Has to be initialized prior any threads spawn.
   * Designed for use only via Boost injector passing by a reference.
   */
  class Ares final {
   public:
    using TxtCallback =
        std::function<void(outcome::result<std::vector<std::string>>)>;

    enum class Error {
      NOT_INITIALIZED = 1,
      CHANNEL_INIT_FAILURE,
      THREAD_FAILED,
      // the following are the codes returned to callback by ::ares_query
      E_NO_DATA,
      E_BAD_QUERY,
      E_SERVER_FAIL,
      E_NOT_FOUND,
      E_SERVER_NOTIMP,
      E_REFUSED,
      E_BAD_NAME,
      E_QUERY_TIMEOUT,
      E_NS_CONN_REFUSED,
      E_NO_MEM,
      E_CANCELLED,
      E_CHANNEL_DESTROYED,
      E_BAD_RESPONSE,
    };

    static inline const std::map<int, Ares::Error> kQueryErrors = {
        {ARES_ENODATA, Error::E_NO_DATA},
        {ARES_EFORMERR, Error::E_BAD_QUERY},
        {ARES_ESERVFAIL, Error::E_SERVER_FAIL},
        {ARES_ENOTFOUND, Error::E_NOT_FOUND},
        {ARES_ENOTIMP, Error::E_SERVER_NOTIMP},
        {ARES_EREFUSED, Error::E_REFUSED},
        {ARES_EBADNAME, Error::E_BAD_NAME},
        {ARES_ETIMEOUT, Error::E_QUERY_TIMEOUT},
        {ARES_ECONNREFUSED, Error::E_NS_CONN_REFUSED},
        {ARES_ENOMEM, Error::E_NO_MEM},
        {ARES_ECANCELLED, Error::E_CANCELLED},
        {ARES_EDESTRUCTION, Error::E_CHANNEL_DESTROYED},
        {ARES_EBADRESP, Error::E_BAD_RESPONSE},
    };

    Ares();
    ~Ares();

    // make it non-copyable
    Ares(const Ares &) = delete;
    Ares(Ares &&) = delete;
    void operator=(const Ares &) = delete;
    void operator=(Ares &&) = delete;

    static void resolveTxt(
        const std::string &uri,
        const std::weak_ptr<boost::asio::io_context> &io_context,
        TxtCallback callback);

   private:
    struct RequestContext {
      std::weak_ptr<boost::asio::io_context> io_context;
      std::string uri;
      TxtCallback callback;

      RequestContext(std::weak_ptr<boost::asio::io_context> io_context_,
                     std::string uri_, TxtCallback callback_)
          : io_context{std::move(io_context_)},
            uri{std::move(uri_)},
            callback{std::move(callback_)} {}
    };

    /// schedules to user's io_context the call of callback with specified error
    static void reportError(
        const std::weak_ptr<boost::asio::io_context> &io_context,
        TxtCallback callback, Error error);

    static void txtCallback(void *arg, int status, int timeouts,
                            unsigned char *abuf, int alen);

    /// does ares sockets processing for the channel, to be in a separate thread
    static void waitAresChannel(::ares_channel channel);

    static void removeRequest(RequestContext *request_ptr);

    static std::atomic_bool initialized_;
    static std::list<std::shared_ptr<RequestContext>> requests_;

    /// Returns "ares" logger
    static log::Logger log();
  };

}  // namespace libp2p::network::c_ares

OUTCOME_HPP_DECLARE_ERROR(libp2p::network::c_ares, Ares::Error);

#endif  // LIBP2P_INCLUDE_LIBP2P_NETWORK_CARES_CARES_HPP
