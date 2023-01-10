/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_read_writer.hpp>

#include <arpa/inet.h>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/common/hexutil.hpp>

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define IO_OUTCOME_TRY_NAME(var, val, res, cb) \
  auto && (var) = (res);                       \
  if ((var).has_error()) {                     \
    cb((var).error());                         \
    return;                                    \
  }                                            \
  auto && (val) = (var).value();

#define IO_OUTCOME_TRY(name, res, cb) \
  IO_OUTCOME_TRY_NAME(UNIQUE_NAME(name), name, res, cb)

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection::websocket, WsReadWriter::Error,
                            e) {
  using E = libp2p::connection::websocket::WsReadWriter::Error;
  switch (e) {
    case E::NOT_ENOUGH_DATA:
      return "Not enough data to complete reading";
    case E::UNEXPECTED_CONTINUE:
      return "Unexpected 'Continue' frame has received";
    case E::INTERNAL_ERROR:
      return "Internal error (reentrancy to the reading)";
    case E::CLOSED:
      return "'Close' frame has received from remote peer";
    case E::UNKNOWN_OPCODE:
      return "Frame with unknown opcode has received";
    default:
      return "Unknown error (WsReadWriter::Error)";
  }
}

namespace libp2p::connection::websocket {
  WsReadWriter::WsReadWriter(
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<connection::LayerConnection> connection,
      gsl::span<const uint8_t> preloaded_data)
      : scheduler_(std::move(scheduler)),
        connection_{std::move(connection)},
        read_buffer_(std::make_shared<common::ByteArray>()),
        read_bytes_(preloaded_data.size()),
        log_(log::createLogger("WsReadWriter")) {
    read_buffer_->resize(
        std::max<size_t>(preloaded_data.size(), kMinBufferSize));
    std::copy(preloaded_data.begin(), preloaded_data.end(),
              read_buffer_->begin());
  }

  void WsReadWriter::read(basic::MessageReadWriter::ReadCallbackFunc cb) {
    if (reading_state_ == ReadingState::WaitHeader) {
      SL_TRACE(log_, "read next portion of data (next frame)");
      read_cb_ = std::move(cb);
      return readFlagsAndPrelen();
    }
    if (reading_state_ == ReadingState::WaitData) {
      SL_TRACE(log_, "read next portion of data (same frame)");
      read_cb_ = std::move(cb);
      return readData();
    }
    SL_TRACE(log_, "can't read next portion of data; reading_state is {}",
             reading_state_);
    cb(Error::INTERNAL_ERROR);  // Reentrancy
  }

  void WsReadWriter::consume(size_t size) {
    std::copy_n(std::next(read_buffer_->begin(), size), read_bytes_ - size,
                read_buffer_->begin());
    read_bytes_ -= size;
  }

  void WsReadWriter::read(size_t size, std::function<void()> cb) {
    SL_TRACE(log_, "read data from buffer or connection");

    if (read_bytes_ >= size) {
      SL_TRACE(log_, "buffer has enough data (has {}, needed {})", read_bytes_,
               size);

      //   |<-----size----->|
      //   |<--------read------>|
      //   |################<<<<|_____________|
      // begin                               end

      cb();
      return;
    }

    SL_TRACE(log_, "buffer does not have enough data (has {}, needed {})",
             read_bytes_, size);

    auto in = gsl::make_span(std::next(read_buffer_->data(), read_bytes_),
                             read_buffer_->size() - read_bytes_);

    SL_TRACE(log_, "try to read from connection upto {} bytes",
             read_buffer_->size() - read_bytes_);

    connection_->readSome(
        in, in.size(),
        [self = shared_from_this(), size,
         cb = std::move(cb)](auto res) mutable {
          if (res.has_error()) {
            SL_DEBUG(self->log_, "Can't read data: {}", res.error().message());
            return self->read_cb_(res.as_failure());
          }
          auto read_bytes = res.value();

          self->read_bytes_ += read_bytes;

          SL_TRACE(self->log_, "read {} more bytes; buffer has {} bytes now",
                   read_bytes, self->read_bytes_);

          if (self->read_bytes_ + read_bytes < size) {
            //   |<---------------size------------->|
            //   |<----read---->|<--new_read-->|
            //   |##############|++++++++++++++++|_____________|
            // begin                                          end

            return self->read(size, std::move(cb));
          }

          //   |<---------size------->|
          //   |<----read---->|<--new_read-->|
          //   |##############|++++++++++++++++|_____________|
          // begin                                          end

          cb();
        });
  }

  void WsReadWriter::readFlagsAndPrelen() {
    SL_TRACE(log_, "read flags and prelen");
    BOOST_ASSERT(reading_state_ == ReadingState::WaitHeader);
    reading_state_ = ReadingState::ReadOpcodeAndPrelen;
    read(2, [self = shared_from_this()]() { self->handleFlagsAndPrelen(); });
  }

  void WsReadWriter::handleFlagsAndPrelen() {
    SL_TRACE(log_, "handle flags and prelen");
    BOOST_ASSERT(reading_state_ == ReadingState::ReadOpcodeAndPrelen);
    BOOST_ASSERT(read_bytes_ >= 2);

    const auto &data = *read_buffer_;
    ctx = decltype(ctx)();
    ctx.finally = static_cast<bool>(data[0] & 0b1000'0000);
    ctx.opcode = static_cast<Opcode>(data[0] & 0b0000'1111);
    ctx.prelen = static_cast<uint8_t>(data[1] & 0b0111'1111);
    ctx.masked = static_cast<bool>(data[1] & 0b1000'0000);

    consume(2);

    reading_state_ = ReadingState::ReadSizeAndMask;
    readSizeAndMask();
  }

  void WsReadWriter::readSizeAndMask() {
    SL_TRACE(log_, "read size and mask");
    BOOST_ASSERT(reading_state_ == ReadingState::ReadSizeAndMask);

    size_t need_to_read = 0;
    if (ctx.prelen < 126) {
      ctx.length = ctx.prelen;
    } else if (ctx.prelen == 126) {
      need_to_read += 2;
    } else if (ctx.prelen == 127) {
      need_to_read += 8;
    } else {
      BOOST_UNREACHABLE_RETURN();
    }
    if (ctx.masked) {
      need_to_read += 4;
    }

    if (need_to_read == 0) {
      return handleSizeAndMask();
    }

    read(need_to_read,
         [self = shared_from_this()]() { self->handleSizeAndMask(); });
  }

  void WsReadWriter::handleSizeAndMask() {
    SL_TRACE(log_, "handle size and mask");
    size_t processed_bytes = 0;

    auto *pos = read_buffer_->data();
    BOOST_ASSERT(ctx.prelen <= 127);
    if (ctx.prelen == 126) {
      uint16_t length = 0;
      processed_bytes += sizeof(length);
      BOOST_ASSERT(read_bytes_ >= processed_bytes);
      std::copy_n(pos, sizeof(length), &length);
      ctx.length = be16toh(length);
      pos = std::next(pos, sizeof(length));
    } else if (ctx.prelen == 127) {
      uint64_t length = 0;
      processed_bytes += sizeof(length);
      BOOST_ASSERT(read_bytes_ >= processed_bytes);
      std::copy_n(pos, sizeof(length), &length);
      ctx.length = be64toh(length);
      pos = std::next(pos, sizeof(length));
    }
    SL_TRACE(log_, "size of frame data is {}", ctx.length);

    if (ctx.masked) {
      processed_bytes += ctx.mask.size();
      BOOST_ASSERT(read_bytes_ >= processed_bytes);
      std::copy_n(pos, ctx.mask.size(), ctx.mask.begin());
      SL_TRACE(log_, "mask of frame is {:02x}{:02x}{:02x}{:02x}", ctx.mask[0],
               ctx.mask[1], ctx.mask[2], ctx.mask[3]);
    } else {
      SL_TRACE(log_, "no mask of frame");
    }

    consume(processed_bytes);

    reading_state_ = ReadingState::HandleHeader;
    handleFrame();
  }

  void WsReadWriter::handleFrame() {
    SL_TRACE(log_, "handle frame");
    BOOST_ASSERT(reading_state_ == ReadingState::HandleHeader);
    if (ctx.opcode == Opcode::Continue) {
      if (last_frame_opcode_ == Opcode::Text
          or last_frame_opcode_ == Opcode::Binary) {
        SL_DEBUG(log_, "Can't handle frame: {}",
                 make_error_code(Error::UNEXPECTED_CONTINUE).message());
        read_cb_(Error::UNEXPECTED_CONTINUE);
        return;
      }
    }

    else if (ctx.opcode == Opcode::Text or ctx.opcode == Opcode::Binary) {
      last_frame_opcode_ = not ctx.finally ? ctx.opcode : Opcode::_undefined;
    }

    else if (ctx.opcode == Opcode::Close) {
      if (not closed_by_remote_) {
        closed_by_remote_ = true;
        SL_DEBUG(log_, "Close-frame is received");
        read_cb_(Error::CLOSED);
        return;
      }
    }

    else {
      SL_DEBUG(log_, "Can't handle frame: {}",
               make_error_code(Error::UNKNOWN_OPCODE).message());
      read_cb_(Error::UNKNOWN_OPCODE);
      return;
    }

    ctx.remaining_data = ctx.length;

    reading_state_ = ReadingState::WaitData;
    readData();
  }

  void WsReadWriter::readData() {
    SL_TRACE(log_, "read data");
    BOOST_ASSERT(reading_state_ == ReadingState::WaitData);
    reading_state_ = ReadingState::ReadData;

    if (ctx.remaining_data > read_buffer_->size()) {
      if (kMaxFrameSize > read_buffer_->size()) {
        read_buffer_->resize(kMaxFrameSize);
      }
    }

    read(1, [self = shared_from_this()]() { self->handleData(); });
  }

  void WsReadWriter::handleData() {
    SL_TRACE(log_, "handle data");
    BOOST_ASSERT(reading_state_ == ReadingState::ReadData);
    reading_state_ = ReadingState::HandleData;

    // if (res.has_error()) {
    //    if (reading_state_ != ReadingState::Idle) {
    //      SL_DEBUG(log_, "Can't read data of frame: {}",
    //      res.error().message()); read_cb_(res.as_failure()); return;
    //    }
    //    if (ctx.opcode == Opcode::Pong) {
    //      SL_DEBUG(log_, "Can't read data of frame: {}",
    //      res.error().message()); pong_cb_(res.as_failure());
    //    }
    //    return;
    // }

    if (ctx.opcode == Opcode::Text or ctx.opcode == Opcode::Binary) {
      auto consuming_data_size = std::min(read_bytes_, ctx.remaining_data);
      auto extra_data_size = read_bytes_ - consuming_data_size;

      auto new_buffer_size = std::max(
          kMinBufferSize,
          std::min(kMaxFrameSize, ctx.remaining_data - consuming_data_size));

      SL_TRACE(log_,
               "> remain_to_read={} "
               "in_buff={} "
               "will_consume={} "
               "extra={} "
               "new_buff_size={}",
               ctx.remaining_data, read_bytes_, consuming_data_size,
               extra_data_size, new_buffer_size);

      // new
      auto buffer = std::make_shared<std::vector<uint8_t>>(new_buffer_size);
      std::swap(read_buffer_, buffer);
      read_bytes_ -= consuming_data_size;

      // copy extra data
      std::copy_n(std::next(buffer->begin(), consuming_data_size),
                  extra_data_size, read_buffer_->begin());

      buffer->resize(consuming_data_size);

      if (ctx.masked) {
        SL_TRACE(log_, "IN_masked {}", common::hex_lower(*buffer));
        // size_t i = 0;
        for (auto &byte : *buffer) {
          // {
          //   size_t mi = ctx.mask_index % ctx.mask.size();
          //   auto b = byte;
          //   auto m = ctx.mask[mi];
          //   auto um = b ^ m;
          //
          //   SL_TRACE(
          //       log_,
          //       "byte[{}]=({:02x}) --> mask[{}]={:02x} ==> '{}' {:02x} {}",
          //       i++, (int)byte, mi, m, (char)((um >= 0x20) ? um : '?'),
          //       (int)(um), (int)(um));
          // }

          byte ^= ctx.mask[ctx.mask_index++ % ctx.mask.size()];
        }
        SL_TRACE(log_, "IN_unmasked {}", common::hex_lower(*buffer));
      } else {
        SL_TRACE(log_, "IN_nomasked {}", common::hex_lower(*buffer));
      }

      scheduler_->schedule([this, cb = std::move(read_cb_),
                            buffer = std::move(buffer)]() mutable {
        SL_TRACE(log_, "return result buff {} bytes", buffer->size());
        SL_TRACE(log_, "IN {}", common::hex_lower(*buffer));
        cb(std::move(buffer));
      });

      ctx.remaining_data -= consuming_data_size;

      reading_state_ = (ctx.remaining_data > 0) ? ReadingState::WaitData
                                                : ReadingState::WaitHeader;
      SL_TRACE(log_,
               "< remain_to_read={} "
               "in_buff={}",
               ctx.remaining_data, read_bytes_);
      return;
    }

    if (ctx.opcode == Opcode::Ping) {
      SL_DEBUG(log_, "Ping-frame data is received");
      //  incoming_ping_data_->insert(incoming_ping_data_->end(),
      //                              read_buffer_->begin(),
      //                              std::next(read_buffer_->begin(),
      //                              read_data));
      //  if (ctx.remaining_data == 0) {
      //    sendPong(*incoming_ping_data_);
      //    reading_state_ = ReadingState::WaitHeader;
      //    readFlagsAndPrelen();
      //  } else {
      //    reading_state_ = ReadingState::WaitData;
      //    readData();
      //  }
      return;
    }

    if (ctx.opcode == Opcode::Pong) {
      SL_DEBUG(log_, "Ping-frame frame is received");
      //  incoming_pong_data_->insert(incoming_pong_data_->end(),
      //                              read_buffer_->begin(),
      //                              std::next(read_buffer_->begin(),
      //                              read_data));
      //  if (ctx.remaining_data == 0) {
      //    pong_cb_(std::move(incoming_pong_data_));
      //    incoming_pong_data_ = std::make_shared<std::vector<uint8_t>>();
      //    reading_state_ = ReadingState::WaitHeader;
      //    readFlagsAndPrelen();
      //  } else {
      //    reading_state_ = ReadingState::WaitData;
      //    readData();
      //  }
      return;
    }

    if (ctx.opcode == Opcode::Close) {
      if (ctx.remaining_data == 0) {
        SL_DEBUG(log_, "Close-frame has received");
        read_cb_(Error::CLOSED);
        pong_cb_(Error::CLOSED);
      } else {
        reading_state_ = ReadingState::WaitData;
        readData();
      }
    }

    BOOST_UNREACHABLE_RETURN();
  }

  void WsReadWriter::sendPing(gsl::span<const uint8_t> buffer,
                              WriteCallbackFunc cb) {
    write(buffer, std::move(cb));
  }

  void WsReadWriter::sendPong(gsl::span<const uint8_t> buffer) {
    write(buffer, [](auto res) {
      // TODO log pong
    });
  }

  void WsReadWriter::sendData() {
    SL_TRACE(log_, "send data");

    bool finally = true;

    // Calculate amount of bytes sending by one frame
    size_t amount = 0;
    for (auto &chunk : writing_queue_) {
      amount += chunk.data.size() - chunk.written_bytes;
      if (amount > kMaxFrameSize) {
        finally = false;  // Will remain some bytes to next frame(s)
      }
      if (amount >= kMaxFrameSize) {
        amount = kMaxFrameSize;
        break;
      }
    }

    if (amount == 0) {
      SL_TRACE(log_, "send data: no data");
      return;
    }

    SL_TRACE(log_, "make frame: {} bytes", amount);

    bool masked = connection_->isInitiator();  // only client is masking data
    std::array<uint8_t, 4> mask{0};
    if (masked) {
      uint32_t mask_int = std::rand();
      std::copy_n(reinterpret_cast<uint8_t *>(&mask_int),  // NOLINT
                  sizeof(mask_int), mask.begin());
      SL_TRACE(log_, "mask of frame is {:02x}{:02x}{:02x}{:02x}", mask[0],
               mask[1], mask[2], mask[3]);
    } else {
      SL_TRACE(log_, "no mask of frame");
    }

    std::vector<uint8_t> frame;

    // clang-format off
    frame.reserve(
      2                                               // header
      + (amount <= 125 ? 0 : amount < 65536 ? 2 : 8)  // extended length
      + (masked ? 4 : 0)                              // mask
      + amount);                                      // data
    // clang-format on

    frame.resize(2);
    // finally flag
    frame[0] = finally ? 0b1000'0000 : 0;
    // opcode
    frame[0] |= static_cast<uint8_t>(Opcode::Binary);
    // masked flag
    frame[1] = masked ? 0b1000'0000 : 0;
    // length
    if (amount < 126) {
      frame[1] |= (amount & 0b0111'1111);
    } else if (amount < 65536) {
      frame[1] |= (126 & 0b0111'1111);
      uint16_t length = amount;
      length = htobe16(length);
      std::copy_n(&length, sizeof(length), std::back_inserter(frame));
    } else {
      frame[1] |= (127 & 0b0111'1111);
      uint64_t length = amount;
      length = htobe64(length);
      std::copy_n(&length, sizeof(length), std::back_inserter(frame));
    }
    // mask
    if (masked) {
      std::copy_n(mask.begin(), mask.size(), std::back_inserter(frame));
    }
    // data
    size_t remaining = amount;
    auto &first_chunk = writing_queue_.front();
    first_chunk.header_size += frame.size();
    for (auto &chunk : writing_queue_) {
      if (remaining == 0) {  // data is over
        break;
      }
      auto chunk_size =
          std::min(chunk.data.size() - chunk.written_bytes, remaining);
      auto chunk_begin = std::next(chunk.data.begin(), chunk.written_bytes);
      auto chunk_end = std::next(chunk_begin, chunk_size);
      if (chunk_begin == chunk_end) {  // empty or completely sent chunk
        continue;
      }
      SL_TRACE(log_, "OUT_orig {}",
               common::hex_lower(gsl::make_span(chunk_begin.base(),
                                                chunk_end - chunk_begin)));
      auto fs = frame.size();
      if (masked) {
        std::transform(chunk_begin, chunk_end, std::back_inserter(frame),
                       [&, idx = 0u](uint8_t byte) mutable {
                         return byte ^ mask[idx++ % mask.size()];
                       });
        SL_TRACE(
            log_, "OUT_masked {}",
            common::hex_lower(gsl::make_span(
                std::next(frame.begin(), fs).base(), chunk_end - chunk_begin)));
      } else {
        std::copy(chunk_begin, chunk_end, std::back_inserter(frame));
        SL_TRACE(
            log_, "OUT_nomask {}",
            common::hex_lower(gsl::make_span(
                std::next(frame.begin(), fs).base(), chunk_end - chunk_begin)));
      }
      chunk.written_bytes += chunk_size;
    }

    // remove processed data out of buffer
    auto write_cb = [self = shared_from_this(),
                     frame_size = frame.size()](auto res) mutable {
      if (res.has_error()) {
        while (not self->writing_queue_.empty()) {
          auto &chunk = self->writing_queue_.front();
          chunk.cb(res.as_failure());
          self->writing_queue_.pop_front();
        }
        return;
      }

      auto written_bytes = res.value();

      SL_TRACE(self->log_, "sent frame: {}", written_bytes);

      if (frame_size != written_bytes) {
        while (not self->writing_queue_.empty()) {
          auto &chunk = self->writing_queue_.front();
          chunk.cb(std::errc::broken_pipe);
          self->writing_queue_.pop_front();
        }
        return;
      }

      while (not self->writing_queue_.empty()) {
        auto &chunk = self->writing_queue_.front();

        const auto sent_header_size =
            std::min(chunk.header_size, written_bytes);
        chunk.header_size -= sent_header_size;
        written_bytes -= sent_header_size;
        SL_TRACE(self->log_, "sent header of frame: {} bytes",
                 sent_header_size);

        const auto sent_data_size = chunk.data.size() - chunk.sent_bytes;
        if (sent_data_size <= written_bytes) {  // completely sent chunk
          SL_TRACE(self->log_,
                   "sent data of frame: chunk completely sent ({} bytes)",
                   sent_data_size);
          chunk.cb(chunk.data.size());
          self->writing_queue_.pop_front();
        } else {  // partially sent chunk
          SL_TRACE(self->log_,
                   "sent data of frame: chunk partially sent ({} bytes)",
                   sent_data_size);
          chunk.sent_bytes += sent_data_size;
          break;
        }
        written_bytes -= sent_data_size;
      }

      for (auto &chunk : self->writing_queue_) {
        if (chunk.data.size() > chunk.written_bytes) {
          self->scheduler_->schedule([wp = self->weak_from_this()] {
            if (auto self = wp.lock()) {
              self->sendData();
            }
          });
        }
      }
    };

    connection_->write(frame, frame.size(), std::move(write_cb));
  }

  void WsReadWriter::write(gsl::span<const uint8_t> buffer,
                           basic::Writer::WriteCallbackFunc cb) {
    SL_TRACE(log_, "write data: {}", buffer.size());

    writing_queue_.emplace_back(
        WritingItem{.data = common::ByteArray(buffer.begin(), buffer.end()),
                    .cb = std::move(cb)});

    scheduler_->schedule([wp = weak_from_this()] {
      if (auto self = wp.lock()) {
        self->sendData();
      }
    });
  }

}  // namespace libp2p::connection::websocket
