/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED

#include <libp2p/common/instances.hpp>

#include <unistd.h>
#include <boost/stacktrace.hpp>
#include <iostream>

namespace libp2p {
  decltype(Instances::mutex) Instances::mutex;
  decltype(Instances::counts) Instances::counts;
}  // namespace libp2p

namespace libp2p::outcome::no_error {
  void report(const void *ptr, size_t size, uint32_t status, const std::type_info &type) {
    std::cerr << "\n\n";
    std::cerr << "no-error bad_result_access " << ptr << "(" << size << ")=0x" << std::hex << status << std::dec << " pid=" << getpid() << " type=" << type.name() << "\n";
    std::cerr << boost::stacktrace::stacktrace{} << "\n";
    while (true) {
      std::cerr.put('\a');
      sleep(1);
    }
  }
}  // namespace libp2p::outcome::no_error
