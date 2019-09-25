/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITER_HPP
#define KAGOME_WRITER_HPP

#include <functional>

#include <boost/system/error_code.hpp>
#include <gsl/span>

namespace libp2p::basic {

  struct Writer {
    using WriteCallback = void(outcome::result<size_t> /*written bytes*/);
    using WriteCallbackFunc = std::function<WriteCallback>;

    virtual ~Writer() = default;

    /**
     * @brief Write exactly {@code} in.size() {@nocode} bytes.
     * Won't call \param cb before all are successfully written.
     * Returns immediately.
     * @param in data to write.
     * @param bytes number of bytes to write
     * @param cb callback with result of operation
     *
     * @note caller should maintain validity of an input buffer until callback
     * is executed. It is usually done with either wrapping buffer as shared
     * pointer, or having buffer as part of some class/struct, and using
     * enable_shared_from_this()
     */
    virtual void write(gsl::span<const uint8_t> in, size_t bytes,
                       WriteCallbackFunc cb) = 0;

    /**
     * @brief Write up to {@code} in.size() {@nocode} bytes.
     * Calls \param cb after only some bytes has been successfully written,
     * so doesn't guarantee that all will be. Returns immediately.
     * @param in data to write.
     * @param bytes number of bytes to write
     * @param cb callback with result of operation
     *
     * @note caller should maintain validity of an input buffer until callback
     * is executed. It is usually done with either wrapping buffer as shared
     * pointer, or having buffer as part of some class/struct, and using
     * enable_shared_from_this()
     */
    virtual void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                           WriteCallbackFunc cb) = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_WRITER_HPP
