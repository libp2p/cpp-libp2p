/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_read_writer.hpp>

#include <arpa/inet.h>

#include <libp2p/common/byteutil.hpp>

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
      return "Unknown error (:WsReadWriter::Error)";
  }
}

namespace libp2p::connection::websocket {
  WsReadWriter::WsReadWriter(
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<connection::LayerConnection> connection,
      std::shared_ptr<common::ByteArray> buffer)
      : scheduler_(std::move(scheduler)),
        connection_{std::move(connection)},
        buffer_{std::move(buffer)},
        log_(log::createLogger("WsReadWriter")) {}

  void WsReadWriter::read(basic::MessageReadWriter::ReadCallbackFunc cb) {
    if (reading_state_ != ReadingState::Idle) {
      cb(Error::INTERNAL_ERROR);  // Reentrancy
      return;
    }
    reading_state_ = ReadingState::WaitHeader;
    read_cb_ = std::move(cb);
    readFlagsAndPrelen();
  }

  void WsReadWriter::readFlagsAndPrelen() {
    BOOST_ASSERT(reading_state_ == ReadingState::WaitHeader);
    reading_state_ = ReadingState::ReadOpcodeAndPrelen;
    connection_->read(*buffer_, 2, [self = shared_from_this()](auto res) {
      self->handleFlagsAndPrelen(res);
    });
  }

  void WsReadWriter::handleFlagsAndPrelen(outcome::result<size_t> res) {
    BOOST_ASSERT(reading_state_ == ReadingState::ReadOpcodeAndPrelen);
    if (res.has_error()) {
      SL_DEBUG(log_, "Can't read flags and length of frame: {}",
               res.error().message());
      read_cb_(res.as_failure());
      return;
    }
    auto read_bytes = res.value();
    if (read_bytes != 2) {
      SL_DEBUG(log_, "Can't read flags and length of frame: {}",
               make_error_code(Error::NOT_ENOUGH_DATA).message());
      read_cb_(Error::NOT_ENOUGH_DATA);
      return;
    }
    const auto &data = *buffer_;
    ctx.finally = static_cast<bool>(data[0] & 0b1000'0000);
    ctx.opcode = static_cast<Opcode>(data[0] & 0b0000'1111);
    ctx.prelen = static_cast<uint8_t>(data[1] & 0b0111'1111);
    ctx.masked = static_cast<bool>(data[1] & 0b1000'0000);

    reading_state_ = ReadingState::ReadSizeAndMask;
    readSizeAndMask();
  }

  void WsReadWriter::readSizeAndMask() {
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

    connection_->read(*buffer_, need_to_read,
                      [self = shared_from_this()](auto res) {
                        self->handleSizeAndMask(res);
                      });
  }

  void WsReadWriter::handleSizeAndMask(outcome::result<size_t> res) {
    if (res.has_error()) {
      SL_DEBUG(log_, "Can't read size and mask of frame: {}",
               res.error().message());
      read_cb_(res.as_failure());
      return;
    }

    auto remaining_bytes = res.value();
    auto *pos = buffer_->data();
    if (ctx.prelen == 126) {
      uint16_t length = 0;
      if (remaining_bytes < sizeof(length)) {
        SL_DEBUG(log_, "Can't read size of frame: {}",
                 make_error_code(Error::NOT_ENOUGH_DATA).message());
        read_cb_(Error::NOT_ENOUGH_DATA);
        return;
      }
      remaining_bytes -= sizeof(length);
      std::copy_n(pos, sizeof(length), &length);
      pos = std::next(pos, sizeof(length));
      ctx.length = be16toh(length);
    } else if (ctx.prelen == 127) {
      uint64_t length = 0;
      if (remaining_bytes < sizeof(length)) {
        SL_DEBUG(log_, "Can't read size of frame: {}",
                 make_error_code(Error::NOT_ENOUGH_DATA).message());
        read_cb_(Error::NOT_ENOUGH_DATA);
        return;
      }
      remaining_bytes -= sizeof(length);
      std::copy_n(pos, sizeof(length), &length);
      pos = std::next(pos, sizeof(length));
      ctx.length = be64toh(length);
    } else {
      BOOST_UNREACHABLE_RETURN();
    }

    if (ctx.masked) {
      if (remaining_bytes < ctx.mask.size()) {
        SL_DEBUG(log_, "Can't read mask of frame: {}",
                 make_error_code(Error::NOT_ENOUGH_DATA).message());
        read_cb_(Error::NOT_ENOUGH_DATA);
        return;
      }
      remaining_bytes -= ctx.mask.size();
      std::copy_n(pos, ctx.mask.size(), ctx.mask.begin());
    }

    if (remaining_bytes != 0) {
      BOOST_UNREACHABLE_RETURN();  // extra data has read
    }

    reading_state_ = ReadingState::HandleHeader;
    handleFrame();
  }

  void WsReadWriter::handleFrame() {
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
    BOOST_ASSERT(reading_state_ = ReadingState::WaitData);
    reading_state_ = ReadingState::ReadData;

    connection_->read(
        *buffer_, ctx.remaining_data,
        [self = shared_from_this()](auto res) { self->handleData(res); });
  }

  void WsReadWriter::handleData(outcome::result<size_t> res) {
    BOOST_ASSERT(reading_state_ = ReadingState::ReadData);
    reading_state_ = ReadingState::HandleData;

    if (res.has_error()) {
      if (reading_state_ != ReadingState::Idle) {
        SL_DEBUG(log_, "Can't read data of frame: {}", res.error().message());
        read_cb_(res.as_failure());
        return;
      }
      if (ctx.opcode == Opcode::Pong) {
        SL_DEBUG(log_, "Can't read data of frame: {}", res.error().message());
        pong_cb_(res.as_failure());
      }
      return;
    }

    auto read_data = res.value();
    BOOST_ASSERT_MSG(read_data <= ctx.remaining_data, "Extra data has read");
    ctx.remaining_data -= read_data;

    if (ctx.opcode == Opcode::Text or ctx.opcode == Opcode::Binary) {
      buffer_->resize(read_data);
      read_cb_(std::move(buffer_));
      buffer_ = std::make_shared<std::vector<uint8_t>>();
      reading_state_ = ReadingState::Idle;
      return;
    }

    if (ctx.opcode == Opcode::Ping) {
      incoming_ping_data_->insert(incoming_ping_data_->end(), buffer_->begin(),
                                  std::next(buffer_->begin(), read_data));
      if (ctx.remaining_data == 0) {
        sendPong(*incoming_ping_data_);
        reading_state_ = ReadingState::WaitHeader;
        readFlagsAndPrelen();
      } else {
        reading_state_ = ReadingState::WaitData;
        readData();
      }
      return;
    }

    if (ctx.opcode == Opcode::Pong) {
      incoming_pong_data_->insert(incoming_pong_data_->end(), buffer_->begin(),
                                  std::next(buffer_->begin(), read_data));
      if (ctx.remaining_data == 0) {
        pong_cb_(std::move(incoming_pong_data_));
        incoming_pong_data_ = std::make_shared<std::vector<uint8_t>>();
        reading_state_ = ReadingState::WaitHeader;
        readFlagsAndPrelen();
      } else {
        reading_state_ = ReadingState::WaitData;
        readData();
      }
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
    bool finally = true;
    bool masked = connection_->isInitiator();  // only client is masking data

    // Calculate amount of bytes sending by one frame
    size_t amount = 0;
    for (auto &chunk : writing_queue_) {
      amount += chunk.data.size() - chunk.sent_bytes;
      if (amount > kMaxFrameSize) {
        finally = false;  // Will remain some bytes to next frame(s)
      }
      if (amount >= kMaxFrameSize) {
        amount = kMaxFrameSize;
        break;
      }
    }

    std::array<uint8_t, 4> mask{0};
    if (masked) {
      uint32_t mask_int = std::rand();
      std::copy_n(reinterpret_cast<uint8_t *>(&mask_int),  // NOLINT
                  sizeof(mask_int), mask.begin());
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
    for (auto &chunk : writing_queue_) {
      if (remaining == 0) {  // data is over
        break;
      }
      auto chunk_size =
          std::min(chunk.data.size() - chunk.sent_bytes, remaining);
      auto chunk_begin = std::next(chunk.data.begin(), chunk.sent_bytes);
      auto chunk_end = std::next(chunk_begin, chunk_size);
      if (chunk_begin == chunk_end) {  // empty or completely sent chunk
        continue;
      }
      if (masked) {
        std::transform(chunk_begin, chunk_end, std::back_inserter(frame),
                       [&, idx = 0u](uint8_t i) mutable {
                         return i ^ mask[idx++ % sizeof(mask)];
                       });
      } else {
        std::copy_n(chunk_begin, amount, std::back_inserter(frame));
      }
    }

    // remove processed data out of buffer
    auto write_cb = [self = shared_from_this(), frame_size = frame.size(),
                     amount](auto res) mutable {
      if (res.has_error()) {
        while (not self->writing_queue_.empty()) {
          auto &chunk = self->writing_queue_.front();
          chunk.cb(res.as_failure());
          self->writing_queue_.pop_front();
        }
        return;
      }

      auto written_bytes = res.value();

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
        auto chunk_size = chunk.data.size() - chunk.sent_bytes;
        if (chunk_size <= amount) {  // completely sent chunk
          amount -= chunk_size;
          chunk.cb(chunk.data.size());
          self->writing_queue_.pop_front();
        } else {  // partially sent chunk
          chunk.sent_bytes += amount;
          break;
        }
      }

      if (not self->writing_queue_.empty()) {
        self->scheduler_->schedule([wp = self->weak_from_this()] {
          if (auto self = wp.lock()) {
            self->sendData();
          }
        });
      }
    };

    connection_->write(frame, frame.size(), std::move(write_cb));
  }

  void WsReadWriter::write(gsl::span<const uint8_t> buffer,
                           basic::Writer::WriteCallbackFunc cb) {
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
