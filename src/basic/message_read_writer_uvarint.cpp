/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/message_read_writer_uvarint.hpp>

#include <vector>

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <libp2p/basic/message_read_writer_error.hpp>
#include <libp2p/basic/read.hpp>
#include <libp2p/basic/varint_reader.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/common/outcome_macro.hpp>
#include <libp2p/multi/uvarint.hpp>

namespace libp2p::basic {
  MessageReadWriterUvarint::MessageReadWriterUvarint(
      std::shared_ptr<ReadWriter> conn)
      : conn_{std::move(conn)} {
    BOOST_ASSERT(conn_ != nullptr);
  }

  void MessageReadWriterUvarint::read(ReadCallbackFunc cb) {
    VarintReader::readVarint(
        conn_,
        [self{shared_from_this()}, cb = std::move(cb)](
            outcome::result<multi::UVarint> varint_res) mutable {
          if (varint_res.has_error()) {
            return cb(varint_res.error());
          }

          auto msg_len = varint_res.value().toUInt64();
          if (0 != msg_len) {
            auto buffer = std::make_shared<std::vector<uint8_t>>(msg_len, 0);
            libp2p::read(self->conn_,
                         *buffer,
                         [self, buffer, cb = std::move(cb)](
                             outcome::result<void> result) mutable {
                           IF_ERROR_CB_RETURN(result);
                           cb(std::move(buffer));
                         });
          } else {
            cb(ResultType{});
          }
        });
  }

  void MessageReadWriterUvarint::write(BytesIn buffer, CbOutcomeVoid cb) {
    auto varint_len = multi::UVarint{static_cast<uint64_t>(buffer.size())};

    auto msg_bytes = std::make_shared<std::vector<uint8_t>>();
    msg_bytes->reserve(varint_len.size() + buffer.size());
    msg_bytes->insert(msg_bytes->end(),
                      std::make_move_iterator(varint_len.toVector().begin()),
                      std::make_move_iterator(varint_len.toVector().end()));
    msg_bytes->insert(msg_bytes->end(), buffer.begin(), buffer.end());

    libp2p::write(conn_,
                  *msg_bytes,
                  [cb = std::move(cb),
                   varint_size = varint_len.size(),
                   msg_bytes](outcome::result<void> result) {
                    IF_ERROR_CB_RETURN(result);
                    cb(outcome::success());
                  });
  }
}  // namespace libp2p::basic
