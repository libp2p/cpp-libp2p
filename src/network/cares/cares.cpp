/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/cares/cares.hpp>

#include <thread>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::network::cares, Ares::Error, e) {
  using E = libp2p::network::cares::Ares::Error;
  switch (e) {
    case E::NOT_INITIALIZED:
      return "C-ares library is not initialized";
    case E::CHANNEL_INIT_FAILURE:
      return "C-ares channel initialization failed";
    case E::E_NO_DATA:
      return "The query completed but contains no answers";
    case E::E_BAD_QUERY:
      return "The query completed but the server claims that the query was "
             "malformed";
    case E::E_SERVER_FAIL:
      return "The query completed but the server claims to have experienced a "
             "failure";
    case E::E_NOT_FOUND:
      return "The query completed but the queried-for domain name was not "
             "found";
    case E::E_SERVER_NOTIMP:
      return "The query completed but the server does not implement the "
             "operation requested by the query";
    case E::E_REFUSED:
      return "The query completed but the server refused the query";
    case E::E_BAD_NAME:
      return "The query name could not be encoded as a domain name, either "
             "because it contained a zero-length label or because it contained "
             "a label of more than 63 characters";
    case E::E_QUERY_TIMEOUT:
      return "No name servers responded within the timeout period";
    case E::E_NS_CONN_REFUSED:
      return "No name servers could be contacted";
    case E::E_NO_MEM:
      return "Memory was exhausted";
    case E::E_CANCELLED:
      return "The query was cancelled";
    case E::E_CHANNEL_DESTROYED:
      return "The name service channel channel is being destroyed; the query "
             "will not be completed";
    case E::E_BAD_RESPONSE:
      return "The response is malformed";
    default:
      return "unknown";
  }
}

namespace libp2p::network::cares {

  std::atomic_bool Ares::initialized_{false};
  std::list<std::shared_ptr<Ares::RequestContext>> Ares::requests_{};
  common::Logger Ares::log_ = common::createLogger("Ares");

  Ares::Ares() {
    bool expected{false};
    bool first_init = initialized_.compare_exchange_strong(expected, true);
    // assert reveals multiple ares initialization which is a misuse
    assert(first_init);
    if (not first_init) {
      // atomic change failed, thus it was already initialized
      log_->debug("C-ares library got initialized more than once");
    }
    if (auto status = ::ares_library_init(ARES_LIB_INIT_ALL);
        ARES_SUCCESS != status) {
      log_->error("Unable to initialize c-ares library - %s",
                  ::ares_strerror(status));
      // on library initialization failure we set initialized = false
      assert(initialized_.compare_exchange_strong(expected, false));
    }
  }

  Ares::~Ares() {
    bool expected{true};
    assert(initialized_.compare_exchange_strong(expected, false));
    ares_library_cleanup();
  }

  void Ares::resolveTxt(std::string uri,
                        std::weak_ptr<boost::asio::io_context> io_context,
                        Ares::TxtCallback callback) {
    if (not initialized_.load()) {
      log_->debug(
          "Unable to execute DNS TXT request to %s due to c-ares library is "
          "not initialized",
          uri);
      reportError(io_context, std::move(callback), Error::NOT_INITIALIZED);
      return;
    }
    int status{ARES_SUCCESS};
    ::ares_options options{};
    ::ares_channel channel{nullptr};
    options.timeout = 30'000;
    status = ares_init_options(&channel, &options, ARES_OPT_TIMEOUTMS);
    if (ARES_SUCCESS != status) {
      log_->debug("Unable to initialize c-ares channel for request to %s - %s",
                  uri, ::ares_strerror(status));
      reportError(io_context, std::move(callback), Error::CHANNEL_INIT_FAILURE);
      return;
    }

    auto request = std::make_shared<RequestContext>(
        std::move(io_context), std::move(uri), std::move(callback));
    requests_.push_back(request);

    std::thread worker([channel, request{std::move(request)}] {
      ::ares_query(channel, request->uri.c_str(), ns_c_in, ns_t_txt,
                   Ares::txtCallback, request.get());
      Ares::waitAresChannel(channel);
      ::ares_destroy(channel);
    });
    worker.detach();
  }

  void Ares::reportError(
      const std::weak_ptr<boost::asio::io_context> &io_context,
      Ares::TxtCallback callback, Ares::Error error) {
    if (auto ctx = io_context.lock()) {
      ctx->post([callback{std::move(callback)}, error] { callback(error); });
      return;
    }
    log_->debug("IO context has expired");
  }

  void Ares::txtCallback(void *arg, int status, int timeouts,
                         unsigned char *abuf, int alen) {
    auto *request_ptr{static_cast<RequestContext *>(arg)};
    auto processError = [request_ptr](int status_code) -> bool {
      if (ARES_SUCCESS != status_code) {
        reportError(request_ptr->io_context, request_ptr->callback,
                    kQueryErrors.at(status_code));
        removeRequest(request_ptr);
        return true;
      }
      return false;
    };
    if (processError(status)) {
      return;
    }
    ::ares_txt_reply *reply{nullptr};
    auto parse_status = ::ares_parse_txt_reply(abuf, alen, &reply);
    if (processError(parse_status)) {
      if (nullptr != reply) {
        ::ares_free_data(reply);
      }
      return;
    }
    std::vector<std::string> result;
    for (::ares_txt_reply *current = reply; current != nullptr;
         current = current->next) {
      std::string txt;
      txt.resize(current->length);
      memcpy(txt.data(), current->txt, current->length);
      result.emplace_back(std::move(txt));
    }
    ::ares_free_data(reply);
    if (auto ctx = request_ptr->io_context.lock()) {
      ctx->post([callback{std::move(request_ptr->callback)},
                 reply{std::move(result)}] {
        callback(reply);
      });
    } else {
      log_->debug("IO context has expired");
    }
    removeRequest(request_ptr);
  }

  void Ares::waitAresChannel(::ares_channel channel) {
    while (true) {
      struct timeval *tvp{nullptr};
      struct timeval tv {};
      fd_set read_fds;
      fd_set write_fds;
      int nfds{0};

      FD_ZERO(&read_fds);
      FD_ZERO(&write_fds);
      nfds = ::ares_fds(channel, &read_fds, &write_fds);
      if (0 == nfds) {
        break;
      }
      tvp = ::ares_timeout(channel, nullptr, &tv);
      select(nfds, &read_fds, &write_fds, nullptr, tvp);
      ::ares_process(channel, &read_fds, &write_fds);
    }
  }

  void Ares::removeRequest(Ares::RequestContext *request_ptr) {
    requests_.remove_if(
        [&](const std::shared_ptr<Ares::RequestContext> &request_sptr) {
          return request_ptr == request_sptr.get();
        });
  }

}  // namespace libp2p::network::cares
