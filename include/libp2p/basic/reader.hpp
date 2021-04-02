/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_READER_HPP
#define LIBP2P_READER_HPP

#include <functional>
#include <vector>

#include <gsl/span>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::basic {

  struct Reader {
    using ReadCallback = void(outcome::result<size_t> /*read bytes*/);
    using ReadCallbackFunc = std::function<ReadCallback>;

    virtual ~Reader() = default;

    /**
     * @brief Reads exactly {@code} min(out.size(), bytes) {@nocode} bytes to
     * the buffer.
     * @param out output argument. Read data will be written to this buffer.
     * @param bytes number of bytes to read
     * @param cb callback with result of operation
     *
     * @note caller should maintain validity of an output buffer until callback
     * is executed. It is usually done with either wrapping buffer as shared
     * pointer, or having buffer as part of some class/struct, and using
     * enable_shared_from_this()
     */
    virtual void read(gsl::span<uint8_t> out, size_t bytes,
                      ReadCallbackFunc cb) = 0;

    /**
     * @brief Reads up to {@code} min(out.size(), bytes) {@nocode} bytes to the
     * buffer.
     * @param out output argument. Read data will be written to this buffer.
     * @param bytes number of bytes to read
     * @param cb callback with result of operation
     *
     * @note caller should maintain validity of an output buffer until callback
     * is executed. It is usually done with either wrapping buffer as shared
     * pointer, or having buffer as part of some class/struct, and using
     * enable_shared_from_this()
     */
    virtual void readSome(gsl::span<uint8_t> out, size_t bytes,
                          ReadCallbackFunc cb) = 0;

    /**
     * @brief Defers reporting result or error to callback to avoid reentrancy
     * (i.e. callback will not be called before initiator function returns)
     * @param res read result
     * @param cb callback
     */
    virtual void deferReadCallback(outcome::result<size_t> res,
                                   ReadCallbackFunc cb) = 0;
  };

}  // namespace libp2p::basic

#endif  // LIBP2P_READER_HPP
