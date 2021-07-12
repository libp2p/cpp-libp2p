/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/mplex/mplex_frame.hpp>

#include <libp2p/basic/varint_reader.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/muxer/mplex/mplexed_connection.hpp>

namespace libp2p::connection {
  common::ByteArray MplexFrame::toBytes() const {
    common::ByteArray result;

    uint64_t id_and_flag = (stream_number << 3) | static_cast<uint8_t>(flag);
    multi::UVarint id_and_flag_varint{id_and_flag};
    multi::UVarint length_varint{length};

    result.insert(result.end(), id_and_flag_varint.toVector().begin(),
                  id_and_flag_varint.toVector().end());
    result.insert(result.end(), length_varint.toVector().begin(),
                  length_varint.toVector().end());
    result.insert(result.end(), data.begin(), data.end());
    return result;
  }

  common::ByteArray createFrameBytes(MplexFrame::Flag flag,
                                     MplexStream::StreamNumber stream_number,
                                     common::ByteArray data) {
    return MplexFrame{flag, stream_number, data.size(), std::move(data)}
        .toBytes();
  }

  outcome::result<MplexFrame> createFrame(uint64_t id_flag,
                                          common::ByteArray data) {
    using Flag = MplexFrame::Flag;

    Flag flag;
    switch (static_cast<uint8_t>(id_flag & 0x07)) {
      case static_cast<uint8_t>(Flag::NEW_STREAM):
        flag = Flag::NEW_STREAM;
        break;
      case static_cast<uint8_t>(Flag::MESSAGE_RECEIVER):
        flag = Flag::MESSAGE_RECEIVER;
        break;
      case static_cast<uint8_t>(Flag::MESSAGE_INITIATOR):
        flag = Flag::MESSAGE_INITIATOR;
        break;
      case static_cast<uint8_t>(Flag::CLOSE_RECEIVER):
        flag = Flag::CLOSE_RECEIVER;
        break;
      case static_cast<uint8_t>(Flag::CLOSE_INITIATOR):
        flag = Flag::CLOSE_INITIATOR;
        break;
      case static_cast<uint8_t>(Flag::RESET_RECEIVER):
        flag = Flag::RESET_RECEIVER;
        break;
      case static_cast<uint8_t>(Flag::RESET_INITIATOR):
        flag = Flag::RESET_INITIATOR;
        break;
      default:
        return RawConnection::Error::CONNECTION_PROTOCOL_ERROR;
    }

    return MplexFrame{flag,
                      static_cast<MplexStream::StreamNumber>(id_flag >> 3),
                      data.size(), std::move(data)};
  }

  void readFrame(std::shared_ptr<basic::ReadWriter> reader,
                 std::function<void(outcome::result<MplexFrame>)> cb) {
    // read first varint
    basic::VarintReader::readVarint(
        reader, [reader, cb{std::move(cb)}](auto &&varint_res) mutable {
          if (varint_res.has_error()) {
            return cb(varint_res.error());
          }

          // read second varint
          basic::VarintReader::readVarint(
              reader,
              [reader, cb{std::move(cb)},
               id_flag =
                   varint_res.value().toUInt64()](auto &&varint_res) mutable {
                if (varint_res.has_error()) {
                  return cb(varint_res.error());
                }

                auto length = varint_res.value().toUInt64();
                if (length == 0) {
                  // no data in this frame
                  return cb(createFrame(id_flag, {}));
                }

                // read data
                std::shared_ptr<common::ByteArray> data =
                    std::make_shared<common::ByteArray>(length, 0);
                reader->read(*data, length,
                             [id_flag, data,
                              cb{std::move(cb)}](auto &&read_res) mutable {
                               if (!read_res) {
                                 return cb(read_res.error());
                               }

                               cb(createFrame(id_flag, std::move(*data)));
                             });
              });
        });
  }
}  // namespace libp2p::connection
