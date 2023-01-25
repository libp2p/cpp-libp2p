/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_read_writer.hpp>

#include <arpa/inet.h>
#include <boost/endian/conversion.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/common/hexutil.hpp>

#define DUMP_WS_DATA 0

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

#if DUMP_WS_DATA
#define LOG_HEX_BYTES(LOG, LVL, LABEL, BEGIN, END)                        \
  if ((LOG)->level() >= (LVL)) {                                          \
    auto _begin = (BEGIN);                                                \
    size_t b = 0, e = 0;                                                  \
    for (auto it = (BEGIN); it <= (END); ++it) {                          \
      if ((e - b) == 100 || it == (END)) {                                \
        (LOG)->log((LVL), "{} {}-{}\t{}", LABEL, b, e,                    \
                   common::hex_lower(gsl::make_span(&*(_begin), e - b))); \
        if (it == (END)) {                                                \
          break;                                                          \
        }                                                                 \
        _begin = it;                                                      \
        b = e;                                                            \
      }                                                                   \
      ++e;                                                                \
    }                                                                     \
  }                                                                       \
  void(0)
#define LOG_RAW_BYTES(LOG, LVL, LABEL, BEGIN, END)                             \
  if ((LOG)->level() >= (LVL)) {                                               \
    auto _begin = (BEGIN);                                                     \
    size_t b = 0, e = 0;                                                       \
    for (auto it = (BEGIN); it <= (END); ++it) {                               \
      if ((e - b) == 100 || it == (END)) {                                     \
        (LOG)->log((LVL), "{} {}-{}\t{}", LABEL, b, e,                         \
                   common::dumpBin(gsl::make_span(&*(_begin), e - b), false)); \
        if (it == (END)) {                                                     \
          break;                                                               \
        }                                                                      \
        _begin = it;                                                           \
        b = e;                                                                 \
      }                                                                        \
      ++e;                                                                     \
    }                                                                          \
  }                                                                            \
  void(0)
#else
#define LOG_HEX_BYTES(LOG, LVL, LABEL, BEGIN, END) void(0)
#define LOG_RAW_BYTES(LOG, LVL, LABEL, BEGIN, END) void(0)
#endif

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

  void WsReadWriter::ping(gsl::span<const uint8_t> payload) {
    // already closed by host
    if (closed_by_host_) {
      return;
    }

    outgoing_ping_data_ =
        std::make_shared<common::ByteArray>(payload.begin(), payload.end());
  }

  void WsReadWriter::setPongHandler(
      std::function<void(gsl::span<const uint8_t>)> handler) {
    read_pong_handler_ = std::move(handler);
  }

  void WsReadWriter::read(basic::MessageReadWriter::ReadCallbackFunc cb) {
    if (reading_state_ == ReadingState::WaitHeader) {
      SL_TRACE(log_, "R: read next portion of data (next frame)");
      read_data_handler_ = std::move(cb);
      return readFlagsAndPrelen();
    }
    if (reading_state_ == ReadingState::WaitData) {
      SL_TRACE(log_, "R: read next portion of data (same frame)");
      read_data_handler_ = std::move(cb);
      return readData();
    }
    if (reading_state_ == ReadingState::Closed) {
      SL_TRACE(log_, "R: can't read - remote closed");
      return cb(Error::CLOSED);
    }
    SL_WARN(log_, "R: can't read next portion of data; reading_state is {}",
            reading_state_);
    cb(Error::INTERNAL_ERROR);  // Reentrancy
  }

  void WsReadWriter::write(gsl::span<const uint8_t> buffer,
                           basic::Writer::WriteCallbackFunc cb) {
    if (closed_by_host_) {
      cb(Error::CLOSED);
      return;
    }

    SL_TRACE(log_, "W: write data: {}", buffer.size());

    LOG_RAW_BYTES(log_, log::Level::TRACE, "W: OUT", buffer.begin(),
                  buffer.end());

    writing_queue_.emplace_back(common::ByteArray(buffer.begin(), buffer.end()),
                                std::move(cb));

    scheduler_->schedule([wp = weak_from_this()] {
      if (auto self = wp.lock()) {
        self->sendData();
      }
    });
  }

  void WsReadWriter::close(ReasonOfClose reason) {
    // already closed by host
    if (closed_by_host_) {
      return;
    }

    bool need_to_send = true;
    uint16_t code = 1001;
    std::string_view message;

    switch (reason) {
      case ReasonOfClose::NORMAL_CLOSE:
        code = 1000;
        message = "";
        break;
      case ReasonOfClose::TOO_LONG_PING_PAYLOAD:
        code = 1002;
        message = "Too long ping payload";
        break;
      case ReasonOfClose::TOO_LONG_PONG_PAYLOAD:
        code = 1002;
        message = "Too long pong payload";
        break;
      case ReasonOfClose::TOO_LONG_CLOSE_PAYLOAD:
        code = 1002;
        message = "Too long close payload";
        break;
      case ReasonOfClose::TOO_LONG_DATA_PAYLOAD:
        code = 1009;
        message = "Too long pong payload";
        break;
      case ReasonOfClose::SOME_DATA_AFTER_CLOSED_BY_REMOTE:
        // Incorrect behavior
        need_to_send = false;
        std::ignore = connection_->close();
        break;
      case ReasonOfClose::PINGING_TIMEOUT:
        code = 1008;
        message = "Pong was not received on time";
        break;
      case ReasonOfClose::UNEXPECTED_CONTINUE:
        code = 1002;
        message = "Unexpected continue-frame";
        break;
      case ReasonOfClose::INTERNAL_ERROR:
        message = "Internal error";
        break;
      case ReasonOfClose::UNKNOWN_OPCODE:
        code = 1002;
        message = "Unsupported opcode has received";
        break;
      case ReasonOfClose::CLOSED:
        // Already closed early
        need_to_send = false;
        break;
    }

    if (need_to_send) {
      boost::endian::native_to_big_inplace(code);

      outgoing_close_data_ =
          std::make_shared<common::ByteArray>(sizeof(code) + message.size());
      std::copy_n(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<uint8_t *>(&code), sizeof(code),
          outgoing_close_data_->begin());
      std::copy_n(message.begin(), message.size(),
                  outgoing_close_data_->begin() + 2);
    }
  }

  void WsReadWriter::consume(size_t size) {
    std::copy_n(std::next(read_buffer_->begin(), static_cast<ssize_t>(size)),
                read_bytes_ - size, read_buffer_->begin());
    read_bytes_ -= size;
  }

  void WsReadWriter::read(size_t size, std::function<void()> cb) {
    SL_TRACE(log_, "R: read data from buffer or connection");

    if (read_bytes_ >= size) {
      SL_TRACE(log_, "R: buffer has enough data (has {}, needed {})",
               read_bytes_, size);

      //   |<-----size----->|
      //   |<--------read------>|
      //   |################<<<<|_____________|
      // begin                               end

      cb();
      return;
    }

    SL_TRACE(log_, "R: buffer does not have enough data (has {}, needed {})",
             read_bytes_, size);

    auto in = gsl::make_span(
        std::next(read_buffer_->data(), static_cast<ssize_t>(read_bytes_)),
        // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
        read_buffer_->size() - read_bytes_);

    SL_TRACE(log_, "R: try to read from connection upto {} bytes",
             read_buffer_->size() - read_bytes_);

    connection_->readSome(
        in, in.size(),
        [self = shared_from_this(), size,
         cb = std::move(cb)](auto res) mutable {
          if (res.has_error()) {
            SL_DEBUG(self->log_, "R: Can't read data: {}",
                     res.error().message());
            if (self->read_data_handler_) {
              self->read_data_handler_(res.as_failure());
            }
            return;
          }
          auto read_bytes = res.value();

          self->read_bytes_ += read_bytes;

          SL_TRACE(self->log_, "R: read {} more bytes; buffer has {} bytes now",
                   read_bytes, self->read_bytes_);

          if (self->read_bytes_ < size) {
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
    SL_TRACE(log_, "R: read flags and prelen");
    BOOST_ASSERT(reading_state_ == ReadingState::WaitHeader);
    reading_state_ = ReadingState::ReadOpcodeAndPrelen;
    read(2, [self = shared_from_this()]() { self->handleFlagsAndPrelen(); });
  }

  void WsReadWriter::handleFlagsAndPrelen() {
    SL_TRACE(log_, "R: handle flags and prelen");
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
    SL_TRACE(log_, "R: read size and mask");
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
    SL_TRACE(log_, "R: handle size and mask");
    size_t processed_bytes = 0;

    auto *pos = read_buffer_->data();
    BOOST_ASSERT(ctx.prelen <= 127);
    if (ctx.prelen == 126) {
      uint16_t length = 0;
      processed_bytes += sizeof(length);
      BOOST_ASSERT(read_bytes_ >= processed_bytes);
      std::copy_n(pos, sizeof(length),
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                  reinterpret_cast<uint8_t *>(&length));
      ctx.length = boost::endian::big_to_native(length);
      pos = std::next(pos, sizeof(length));
    } else if (ctx.prelen == 127) {
      uint64_t length = 0;
      processed_bytes += sizeof(length);
      BOOST_ASSERT(read_bytes_ >= processed_bytes);
      std::copy_n(pos, sizeof(length),
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                  reinterpret_cast<uint8_t *>(&length));
      ctx.length = boost::endian::big_to_native(length);
      pos = std::next(pos, sizeof(length));
    }
    SL_TRACE(log_, "R: size of frame data is {}", ctx.length);

    if (ctx.masked) {
      processed_bytes += ctx.mask.size();
      BOOST_ASSERT(read_bytes_ >= processed_bytes);
      std::copy_n(pos, ctx.mask.size(), ctx.mask.begin());
      SL_TRACE(log_, "R: mask of frame is {:02x}{:02x}{:02x}{:02x}",
               ctx.mask[0], ctx.mask[1], ctx.mask[2], ctx.mask[3]);
    } else {
      SL_TRACE(log_, "R: no mask of frame");
    }

    consume(processed_bytes);
    if (read_buffer_->size() < ctx.length) {
      read_buffer_->resize(std::min(ctx.length, kMaxFrameSize));
    }

    reading_state_ = ReadingState::HandleHeader;
    handleFrame();
  }

  void WsReadWriter::handleFrame() {
    SL_TRACE(log_, "R: handle frame");
    BOOST_ASSERT(reading_state_ == ReadingState::HandleHeader);
    if (ctx.opcode == Opcode::Continue) {
      if (last_frame_opcode_ == Opcode::Text
          or last_frame_opcode_ == Opcode::Binary) {
        SL_DEBUG(log_, "R: Can't handle frame: {}",
                 make_error_code(Error::UNEXPECTED_CONTINUE).message());
        close(ReasonOfClose::UNEXPECTED_CONTINUE);
        if (read_data_handler_) {
          read_data_handler_(Error::UNEXPECTED_CONTINUE);
        }
        return;
      }
    }

    else if (ctx.opcode == Opcode::Text or ctx.opcode == Opcode::Binary) {
      last_frame_opcode_ = not ctx.finally ? ctx.opcode : Opcode::_undefined;
    }

    else if (ctx.opcode != Opcode::Ping and  //
             ctx.opcode != Opcode::Pong and  //
             ctx.opcode != Opcode::Close) {
      SL_DEBUG(log_, "R: Can't handle frame: unknown opcode 0x{:X}",
               ctx.opcode);
      close(ReasonOfClose::UNKNOWN_OPCODE);
      if (read_data_handler_) {
        read_data_handler_(Error::UNKNOWN_OPCODE);
      }
      return;
    }

    ctx.remaining_data = ctx.length;

    reading_state_ = ReadingState::WaitData;
    readData();
  }

  void WsReadWriter::readData() {
    SL_TRACE(log_, "R: read data");
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
    SL_TRACE(log_, "R: handle data");
    BOOST_ASSERT(reading_state_ == ReadingState::ReadData);
    reading_state_ = ReadingState::HandleData;

    auto consuming_data_size = std::min(read_bytes_, ctx.remaining_data);
    auto extra_data_size = read_bytes_ - consuming_data_size;

    auto new_buffer_size =
        std::min(kMaxFrameSize, ctx.remaining_data - consuming_data_size)
        + kMinBufferSize;

    SL_TRACE(log_,
             "R: > remain_to_read={} "
             "in_buff={} "
             "will_consume={} "
             "extra={} "
             "new_buff_size={}",
             ctx.remaining_data, read_bytes_, consuming_data_size,
             extra_data_size, new_buffer_size);

    // [read:[consuming|extra|     ]]

    // new
    auto buffer = std::make_shared<std::vector<uint8_t>>(new_buffer_size);

    // [read:[consuming|extra|     ]]
    // [buff:[                     ]]

    std::swap(read_buffer_, buffer);

    // [read:[                     ]]
    // [buff:[consuming|extra|     ]]

    // copy extra data to new buffer
    std::copy_n(
        std::next(buffer->begin(), static_cast<ssize_t>(consuming_data_size)),
        extra_data_size, read_buffer_->begin());

    // [read:[extra|               ]]
    // [buff:[consuming|extra|     ]]

    // drop extra data
    buffer->resize(consuming_data_size);

    // [read:[extra|               ]]
    // [buff:[consuming|           ]]

    read_bytes_ -= consuming_data_size;
    BOOST_ASSERT(read_bytes_ == extra_data_size);

    LOG_RAW_BYTES(log_, log::Level::TRACE, "R: IN_frame", buffer->begin(),
                  buffer->end());

    if (ctx.masked) {
      for (auto &byte : *buffer) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        byte ^= ctx.mask[ctx.mask_index++ % ctx.mask.size()];
      }
    }

    ctx.remaining_data -= consuming_data_size;

    reading_state_ = (ctx.remaining_data > 0) ? ReadingState::WaitData
                                              : ReadingState::WaitHeader;
    SL_TRACE(log_,
             "R: < remain_to_read={} "
             "in_buff={}",
             ctx.remaining_data, read_bytes_);

    if (ctx.opcode == Opcode::Text or ctx.opcode == Opcode::Binary) {
      scheduler_->schedule([log = log_, cb = std::move(read_data_handler_),
                            buffer = std::move(buffer)]() mutable {
        SL_TRACE(log, "R: return result buff {} bytes", buffer->size());

        LOG_RAW_BYTES(log, log::Level::TRACE, "R: IN", buffer->begin(),
                      buffer->end());

        cb(std::move(buffer));
      });

      return;
    }

    if (ctx.opcode == Opcode::Ping) {
      incoming_control_data_.insert(incoming_control_data_.end(),
                                    buffer->begin(), buffer->end());
      if (incoming_control_data_.size() > kMaxControlFrameDataSize) {
        SL_TRACE(log_, "R: Received ping-frame is too long");
        close(ReasonOfClose::TOO_LONG_PING_PAYLOAD);
        return;
      }

      if (ctx.remaining_data == 0) {
        SL_DEBUG(log_, "R: Ping-frame is received");
        outgoing_pong_data_ = std::make_shared<common::ByteArray>(
            std::move(incoming_control_data_));
        reading_state_ = ReadingState::WaitHeader;
        readFlagsAndPrelen();
      } else {
        reading_state_ = ReadingState::WaitData;
        readData();
      }
      return;
    }

    if (ctx.opcode == Opcode::Pong) {
      incoming_control_data_.insert(incoming_control_data_.end(),
                                    buffer->begin(), buffer->end());
      if (incoming_control_data_.size() > kMaxControlFrameDataSize) {
        SL_TRACE(log_, "R: Received pong-frame is too long");
        close(ReasonOfClose::TOO_LONG_PONG_PAYLOAD);
        return;
      }

      if (ctx.remaining_data == 0) {
        SL_DEBUG(log_, "R: Pong-frame is received");
        read_pong_handler_(incoming_control_data_);
        incoming_control_data_.clear();
        reading_state_ = ReadingState::WaitHeader;
        readFlagsAndPrelen();
      } else {
        reading_state_ = ReadingState::WaitData;
        readData();
      }
      return;
    }

    if (ctx.opcode == Opcode::Close) {
      incoming_control_data_.insert(incoming_control_data_.end(),
                                    buffer->begin(), buffer->end());
      if (incoming_control_data_.size() > kMaxControlFrameDataSize) {
        SL_TRACE(log_, "R: Received close-frame is too long");
        close(ReasonOfClose::TOO_LONG_CLOSE_PAYLOAD);
        return;
      }

      if (ctx.remaining_data == 0) {
        if (incoming_control_data_.size() >= 2) {
          uint16_t code;  // NOLINT(cppcoreguidelines-init-variables)
          std::copy_n(
              incoming_control_data_.begin(), 2,
              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
              reinterpret_cast<uint8_t *>(&code));
          if (incoming_control_data_.size() > 2) {
            std::string_view msg(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                reinterpret_cast<char *>(incoming_control_data_.data()),
                incoming_control_data_.size());
            SL_DEBUG(log_, "R: Close-frame is received; code: {}, message: {}",
                     code, msg);
          } else {
            SL_DEBUG(log_, "R: Close-frame is received; code: {}", code);
          }
        } else {
          SL_DEBUG(log_, "R: Close-frame is received");
        }
        closed_by_remote_ = true;
        reading_state_ = ReadingState::Closed;
        shutdown(Error::CLOSED);
      } else {
        reading_state_ = ReadingState::WaitData;
        readData();
      }
    }

    return BOOST_UNREACHABLE_RETURN();
  }

  bool WsReadWriter::hasOutgoingData() {
    // Ready to send control frame
    if (outgoing_close_data_ or outgoing_pong_data_ or outgoing_ping_data_) {
      return true;
    }

    // Exists unsent data
    return std::all_of(
        writing_queue_.begin(), writing_queue_.end(),
        [](auto &chunk) { return chunk.data.size() > chunk.written_bytes; });
  }

  void WsReadWriter::shutdown(outcome::result<void> res) {
    BOOST_ASSERT(res.has_error());

    // Drop control frame data
    outgoing_close_data_.reset();
    outgoing_pong_data_.reset();
    outgoing_ping_data_.reset();

    // Drop unsent data
    for (auto &chunk : writing_queue_) {
      chunk.cb(res.as_failure());
    }
    writing_queue_.clear();

    // callback of reading
    if (read_data_handler_) {
      read_data_handler_(res.as_failure());
    }

    std::ignore = connection_->close();
  }

  void WsReadWriter::sendData() {
    if (outgoing_close_data_) {
      sendControlFrame(ControlFrameType::Close,
                       std::move(outgoing_close_data_));
      return;
    }

    if (outgoing_pong_data_) {
      sendControlFrame(ControlFrameType::Pong, std::move(outgoing_pong_data_));
      return;
    }

    if (outgoing_ping_data_) {
      sendControlFrame(ControlFrameType::Ping, std::move(outgoing_ping_data_));
      return;
    }

    sendDataFrame();
  }

  void WsReadWriter::sendControlFrame(
      ControlFrameType type, std::shared_ptr<common::ByteArray> payload) {
    BOOST_ASSERT(type == ControlFrameType::Ping
                 or type == ControlFrameType::Pong
                 or type == ControlFrameType::Close);

    const auto *marker = type == ControlFrameType::Ping ? "ping"
        : type == ControlFrameType::Pong                ? "pong"
                                                        : "close";
    const auto opcode = type == ControlFrameType::Ping ? Opcode::Ping
        : type == ControlFrameType::Pong               ? Opcode::Pong
                                                       : Opcode::Close;

    bool finally = true;

    size_t amount = payload->size();

    if (amount == 0) {
      SL_TRACE(log_, "W: send {}: no data", marker);
      return;
    }

    if (amount > kMaxControlFrameDataSize) {
      close(ReasonOfClose::INTERNAL_ERROR);
      sendControlFrame(ControlFrameType::Close,
                       std::move(outgoing_close_data_));
      return;
    }

    SL_TRACE(log_, "W: make {}-frame: {} bytes", marker, amount);

    bool masked = false;  // only client is masking data
    std::array<uint8_t, 4> mask{0};
    if (masked) {
      uint32_t mask_int = std::rand();
      std::copy_n(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<uint8_t *>(&mask_int), sizeof(mask_int),
          mask.begin());
      SL_TRACE(log_, "W: mask of frame is {:02x}{:02x}{:02x}{:02x}", mask[0],
               mask[1], mask[2], mask[3]);
    } else {
      SL_TRACE(log_, "W: no mask of frame");
    }

    auto out = std::make_shared<common::ByteArray>();
    auto &frame = *out;

    frame.reserve(2                   // header
                  + (masked ? 4 : 0)  // mask
                  + amount);          // data

    frame.resize(2);
    frame[0] = finally ? 0b1000'0000 : 0;      // finally flag
    frame[0] |= static_cast<uint8_t>(opcode);  // opcode
    frame[1] = masked ? 0b1000'0000 : 0;       // masked flag
    frame[1] |= (amount & 0b0111'1111);        // length
    // mask
    if (masked) {
      std::copy_n(mask.begin(), mask.size(), std::back_inserter(frame));
    }

    // data
    if (masked) {
      std::transform(payload->begin(), payload->end(),
                     std::back_inserter(frame),
                     [&, idx = 0u](uint8_t byte) mutable {
                       // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                       return byte ^ mask[idx++ % mask.size()];
                     });
    } else {
      frame.insert(frame.end(), payload->begin(), payload->end());
    }

    auto write_cb = [self = shared_from_this(), out, marker,
                     opcode](auto res) mutable {
      if (res.has_error()) {
        self->shutdown(res.as_failure());
        return;
      }

      auto written_bytes = res.value();

      if (out->size() != written_bytes) {
        self->shutdown(std::errc::broken_pipe);
        return;
      }

      SL_TRACE(self->log_, "W: sent {}-frame: {}", marker, written_bytes);

      if (opcode == Opcode::Close) {
        self->shutdown(Error::CLOSED);
        return;
      }

      if (self->hasOutgoingData()) {
        self->scheduler_->schedule([wp = self->weak_from_this()] {
          if (auto self = wp.lock()) {
            self->sendData();
          }
        });
      }
    };

    connection_->write(*out, out->size(), std::move(write_cb));

    closed_by_host_ = true;
  }

  void WsReadWriter::sendDataFrame() {
    bool finally = true;
    size_t amount = 0;

    // Calculate amount of bytes sending by one frame
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
      return;
    }

    SL_TRACE(log_, "W: send data");

    SL_TRACE(log_, "W: make frame: {} bytes", amount);

    bool masked = connection_->isInitiator();  // only client is masking data
    std::array<uint8_t, 4> mask{0};
    if (masked) {
      uint32_t mask_int = std::rand();
      std::copy_n(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<uint8_t *>(&mask_int), sizeof(mask_int),
          mask.begin());
      SL_TRACE(log_, "W: mask of frame is {:02x}{:02x}{:02x}{:02x}", mask[0],
               mask[1], mask[2], mask[3]);
    } else {
      SL_TRACE(log_, "W: no mask of frame");
    }

    auto out = std::make_shared<common::ByteArray>();
    auto &frame = *out;

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
      boost::endian::native_to_big_inplace(length);
      std::copy_n(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<uint8_t *>(&length), sizeof(length),
          std::back_inserter(frame));
    } else {
      frame[1] |= (127 & 0b0111'1111);
      uint64_t length = amount;
      boost::endian::native_to_big_inplace(length);
      std::copy_n(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<uint8_t *>(&length), sizeof(length),
          std::back_inserter(frame));
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
      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
      auto chunk_begin = std::next(chunk.data.begin(), chunk.written_bytes);
      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
      auto chunk_end = std::next(chunk_begin, chunk_size);
      if (chunk_begin == chunk_end) {  // empty or completely sent chunk
        continue;
      }

      if (masked) {
        std::transform(chunk_begin, chunk_end, std::back_inserter(frame),
                       [&, idx = 0u](uint8_t byte) mutable {
                         // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                         return byte ^ mask[idx++ % mask.size()];
                       });
      } else {
        std::copy(chunk_begin, chunk_end, std::back_inserter(frame));
      }

      chunk.written_bytes += chunk_size;
    }

    LOG_RAW_BYTES(log_, log::Level::TRACE, "W: OUT_frame", frame.begin(),
                  frame.end());

    // remove processed data out of buffer
    auto write_cb = [self = shared_from_this(), out](auto res) mutable {
      if (res.has_error()) {
        self->shutdown(res.as_failure());
        return;
      }

      auto written_bytes = res.value();

      SL_TRACE(self->log_, "W: sent data-frame: {}", written_bytes);

      if (out->size() != written_bytes) {
        self->shutdown(std::errc::broken_pipe);
        return;
      }

      while (not self->writing_queue_.empty()) {
        auto &chunk = self->writing_queue_.front();

        if (chunk.header_size > 0) {
          const auto sent_header_size =
              std::min(chunk.header_size, written_bytes);
          chunk.header_size -= sent_header_size;
          written_bytes -= sent_header_size;
          SL_TRACE(self->log_, "W: sent header of frame: {} bytes",
                   sent_header_size);
        }

        if (written_bytes == 0) {
          break;
        }

        const auto sent_data_size =
            std::min(chunk.written_bytes - chunk.sent_bytes, written_bytes);
        chunk.sent_bytes += sent_data_size;
        written_bytes -= sent_data_size;

        if (chunk.sent_bytes == chunk.data.size()) {  // completely sent chunk
          SL_TRACE(
              self->log_,
              "W: sent data of frame: chunk completely sent: +{} bytes ({}/{})",
              sent_data_size, chunk.sent_bytes, chunk.data.size());
          chunk.cb(chunk.data.size());
          self->writing_queue_.pop_front();
        } else {  // partially sent chunk
          SL_TRACE(
              self->log_,
              "W: sent data of frame: chunk partially sent: +{} bytes ({}/{})",
              sent_data_size, chunk.sent_bytes, chunk.data.size());
          break;
        }
      }

      if (self->hasOutgoingData()) {
        self->scheduler_->schedule([wp = self->weak_from_this()] {
          if (auto self = wp.lock()) {
            self->sendData();
          }
        });
      }
    };

    connection_->write(*out, out->size(), std::move(write_cb));
  }

}  // namespace libp2p::connection::websocket
