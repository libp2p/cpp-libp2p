/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <string>
#include <thread>

#include <ares.h>
#include <arpa/nameser.h>
#include <sys/select.h>

static void callback_txt(void *arg, int status, int timeouts,
                         unsigned char *abuf, int alen) {
  if (ARES_SUCCESS != status) {
    std::cout << "query failure, status: " << status << std::endl;
  }
  std::cout << "query ok" << std::endl;
  ares_txt_reply *reply = nullptr;
  status = ares_parse_txt_reply(abuf, alen, &reply);
  if (ARES_SUCCESS != status) {
    std::cout << "parse txt record failure: " << ares_strerror(status)
              << std::endl;
  }
  for (ares_txt_reply *current = reply; current != nullptr;
       current = current->next) {
    std::string txt;
    txt.resize(current->length);
    memcpy(txt.data(), current->txt, current->length);
    std::cout << "TXT: " << txt << std::endl;
  }
  ares_free_data(reply);
}

static void wait_ares(ares_channel channel) {
  for (;;) {
    struct timeval *tvp = nullptr;
    struct timeval tv{};
    fd_set read_fds;
    fd_set write_fds;
    int nfds = 0;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    nfds = ares_fds(channel, &read_fds, &write_fds);
    if (nfds == 0) {
      break;
    }
    tvp = ares_timeout(channel, nullptr, &tv);
    select(nfds, &read_fds, &write_fds, nullptr, tvp);
    ares_process(channel, &read_fds, &write_fds);
  }
}

int main() {
  int status{0};
  ares_channel channel = nullptr;
  ares_options options{};

  status = ares_library_init(ARES_LIB_INIT_ALL);
  if (ARES_SUCCESS != status) {
    std::cout << "c-ares init failure: " << ares_strerror(status) << std::endl;
    return 1;
  }

  options.timeout = 30000;
  status = ares_init_options(&channel, &options, ARES_OPT_TIMEOUTMS);
  if (ARES_SUCCESS != status) {
    std::cout << "c-ares init options failure: " << ares_strerror(status)
              << std::endl;
  }

  std::thread sleepy([&channel] {
    ares_query(channel, "_dnsaddr.bootstrap.libp2p.io", ns_c_in, ns_t_txt,
               callback_txt, nullptr);
    wait_ares(channel);
  });

  sleepy.join();
  ares_destroy(channel);
  std::cout << "done" << std::endl;
  ares_library_cleanup();
  return 0;
}
