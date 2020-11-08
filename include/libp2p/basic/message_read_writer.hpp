/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MESSAGE_READ_WRITER_HPP
#define LIBP2P_MESSAGE_READ_WRITER_HPP

#include <memory>
#include <vector>

#include <gsl/span>
#include <libp2p/basic/readwriter.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::basic {
  /**
   * An interface for message read writers for messages prepended with its
   * length that is somehow encoded.
   */
  class MessageReadWriter {
   public:
    using ResultType = std::shared_ptr<std::vector<uint8_t>>;
    using ReadCallback = outcome::result<ResultType>;
    using ReadCallbackFunc = std::function<void(ReadCallback)>;

    virtual ~MessageReadWriter() = default;

    /**
     * Reads a message that is prepended with its length
     * @param cb is called when read gets done. Read bytes or an error will be
     * passed as an argument in case of success
     */
    virtual void read(ReadCallbackFunc cb) = 0;

    /**
     * Writes a message and preprends its length
     * @param buffer - bytes to be written
     * @param cb is called when the message is written or an error happened.
     * Quantity of bytes written is passed as an argument in case of success
     */
    virtual void write(gsl::span<const uint8_t> buffer,
                       Writer::WriteCallbackFunc cb) = 0;
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_MESSAGE_READ_WRITER_HPP
