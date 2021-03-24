/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/cares/cares.hpp>

#include <cstring>
#include <stdexcept>
#include <thread>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::network::c_ares, Ares::Error, e) {
  using E = libp2p::network::c_ares::Ares::Error;
  switch (e) {
    case E::NOT_INITIALIZED:
      return "C-ares library is not initialized";
    case E::CHANNEL_INIT_FAILURE:
      return "C-ares channel initialization failed";
    case E::THREAD_FAILED:
      return "Ares unable to start worker thread";
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

namespace libp2p::network::c_ares {

  // linting of the three lines is disabled due to clang-tidy bug
  // https://bugs.llvm.org/show_bug.cgi?id=48040
  std::atomic_bool Ares::initialized_{false};                          // NOLINT
  std::list<std::shared_ptr<Ares::RequestContext>> Ares::requests_{};  // NOLINT

  log::Logger Ares::log() {
    static log::Logger logger = log::createLogger("Ares");
    return logger;
  }

  Ares::Ares() {
    bool expected{false};
    bool first_init = initialized_.compare_exchange_strong(expected, true);
    if (not first_init) {
      // atomic change failed, thus it was already initialized
      SL_DEBUG(log(), "C-ares library got initialized more than once");
    } else if (auto status = ::ares_library_init(ARES_LIB_INIT_ALL);
               ARES_SUCCESS != status) {
      log()->error("Unable to initialize c-ares library - {}",
                   ::ares_strerror(status));
      // on library initialization failure we set initialized = false
      [[maybe_unused]] bool switched_back_on_error{
          initialized_.compare_exchange_strong(expected, false)};
      assert(switched_back_on_error);
    }
  }

  Ares::~Ares() {
    bool expected{true};
    if (initialized_.compare_exchange_strong(expected, false)) {
      ares_library_cleanup();
    }
  }

  void Ares::resolveTxt(
      const std::string &uri,
      const std::weak_ptr<boost::asio::io_context> &io_context,
      Ares::TxtCallback callback) {
    if (not initialized_.load()) {
      SL_DEBUG(
          log(),
          "Unable to execute DNS TXT request to {} due to c-ares library is "
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
      SL_DEBUG(log(),
               "Unable to initialize c-ares channel for request to {} - {}",
               uri, ::ares_strerror(status));
      reportError(io_context, std::move(callback), Error::CHANNEL_INIT_FAILURE);
      return;
    }

    auto request =
        std::make_shared<RequestContext>(io_context, uri, std::move(callback));
    requests_.push_back(request);

    try {
      std::thread worker([channel, request] {
        ::ares_query(channel, request->uri.c_str(), ns_c_in, ns_t_txt,
                     Ares::txtCallback, request.get());
        Ares::waitAresChannel(channel);
        ::ares_destroy(channel);
      });
      worker.detach();
    } catch (const std::runtime_error &e) {
      log()->error("Ares unable to start worker thread - {}", e.what());
      request->callback(Error::THREAD_FAILED);
    }
  }

  void Ares::reportError(
      const std::weak_ptr<boost::asio::io_context> &io_context,
      Ares::TxtCallback callback, Ares::Error error) {
    if (auto ctx = io_context.lock()) {
      ctx->post([callback{std::move(callback)}, error] { callback(error); });
      return;
    }
    SL_DEBUG(log(), "IO context has expired");
  }

  void Ares::txtCallback(void *arg, int status, int, unsigned char *abuf,
                         int alen) {
    auto *request_ptr{static_cast<RequestContext *>(arg)};
    if (ARES_SUCCESS != status) {
      reportError(request_ptr->io_context, request_ptr->callback,
                  kQueryErrors.at(status));
      removeRequest(request_ptr);
      return;
    }
    ::ares_txt_reply *reply{nullptr};
    auto parse_status = ::ares_parse_txt_reply(abuf, alen, &reply);
    if (ARES_SUCCESS != parse_status) {
      reportError(request_ptr->io_context, request_ptr->callback,
                  kQueryErrors.at(parse_status));
      removeRequest(request_ptr);
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
      std::memcpy(txt.data(), current->txt, current->length);
      result.emplace_back(std::move(txt));
    }
    ::ares_free_data(reply);
    if (auto ctx = request_ptr->io_context.lock()) {
      ctx->post([callback{std::move(request_ptr->callback)},
                 reply{std::move(result)}] { callback(reply); });
    } else {
      SL_DEBUG(log(), "IO context has expired");
    }
    removeRequest(request_ptr);
  }

  void Ares::waitAresChannel(::ares_channel channel) {
    while (true) {
      struct timeval *tvp{nullptr};  // NOLINT
      struct timeval tv {};
      fd_set read_fds;
      fd_set write_fds;
      int nfds{0};  // NOLINT

      FD_ZERO(&read_fds);   // NOLINT
      FD_ZERO(&write_fds);  // NOLINT
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

}  // namespace libp2p::network::c_ares
