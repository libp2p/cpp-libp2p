
/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GMOCK_ACTIONS_HPP
#define LIBP2P_GMOCK_ACTIONS_HPP

#include <gmock/gmock.h>
#include <boost/system/error_code.hpp>

/**
 * @code
 * const int size = 1;
 * EXPECT_CALL(*connection_, read(_, _, _)).WillOnce(AsioSuccess(size));
 * auto buf = std::make_shared<std::vector<uint8_t>>(size, 0);
 * secure_connection_->read(*buf, size, [&size, buf](auto &&res) mutable {
 *   ASSERT_TRUE(res) << res.error().message();
 *   ASSERT_EQ(read, size);
 * });
 * @endcode
 */
ACTION_P(AsioSuccess, size) {
  // arg0 - buffer
  // arg1 - bytes
  // arg2 - callback
  arg2(size);
}

/**
 * @code
 * const int size = 1;
 * boost::system::error_code ec = ...;
 * EXPECT_CALL(*connection_, read(_, _, _)).WillOnce(AsioCallback(ec, size));
 * auto buf = std::make_shared<std::vector<uint8_t>>(size, 0);
 * secure_connection_->read(*buf, size, [&size, buf, e=ec](auto &&ec, size_t
 * read) mutable { ASSERT_EQ(ec.value(), e.value()); ASSERT_EQ(read, size);
 * });
 * @endcode
 */
ACTION_P2(AsioCallback, ec, size) {
  // arg0 - buffer
  // arg1 - bytes
  // arg2 - callback
  arg2(ec, size);
}

ACTION_P(Arg0CallbackWithArg, in) {
  arg0(in);
}

ACTION_P(Arg1CallbackWithArg, in) {
  arg1(in);
}

ACTION_P(Arg2CallbackWithArg, in) {
  arg2(in);
}

ACTION_P(Arg3CallbackWithArg, in) {
  arg3(in);
}

ACTION_P(Arg4CallbackWithArg, in) {
  arg4(in);
}

ACTION_P(UpgradeToSecureInbound, do_upgrade) {
  arg1(do_upgrade(arg0));
}

ACTION_P(UpgradeToSecureOutbound, do_upgrade) {
  arg2(do_upgrade(arg0));
}

ACTION_P(UpgradeToMuxed, do_upgrade) {
  arg1(do_upgrade(arg0));
}

#endif  // LIBP2P_GMOCK_ACTIONS_HPP
