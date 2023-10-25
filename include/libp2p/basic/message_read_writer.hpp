/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include <libp2p/basic/readwriter.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <span>

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
    virtual void write(BytesIn buffer, Writer::WriteCallbackFunc cb) = 0;
  };
}  // namespace libp2p::basic
