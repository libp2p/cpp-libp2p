/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_LAYER_WEBSOCKET_WSREADWRITTER
#define LIBP2P_LAYER_WEBSOCKET_WSREADWRITTER

#include <libp2p/basic/message_read_writer.hpp>

#include <deque>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/connection/layer_connection.hpp>
#include <libp2p/log/logger.hpp>

namespace libp2p::connection::websocket {

  /**
   * Does raw messages exchange, primary during handshake.
   * Implements transparent messages prefixing with length marker (16bytes big
   * endian prefix).
   *
   * Does __NOT__ destroy connection during destruction and buffer is not going
   * to be destroyed too.
   */
  class WsReadWriter : public basic::MessageReadWriter,
                       public std::enable_shared_from_this<WsReadWriter> {
   public:
    using WriteCallbackFunc = basic::Writer::WriteCallbackFunc;

    // Max size of frame header
    // 1 - flags and opcode
    // 1 - mask flag and pre-length
    // 8 - max-size of extended length
    // 4 - mask
    // 125 - max non-extended length
    static constexpr size_t kMinBufferSize = 139;

    static constexpr size_t kMaxFrameSize = 1 << 20;  // 1Mb

    static constexpr size_t kMaxControlFrameDataSize = 125;  // By RFC6455

    /**
     * Initializes read writer
     * @param connection - raw connection
     * @param buffer - pointer to buffer where to store bytes from network
     */
    WsReadWriter(std::shared_ptr<basic::Scheduler> scheduler,
                 std::shared_ptr<connection::LayerConnection> connection,
                 gsl::span<const uint8_t> preloaded_data);

    void ping(gsl::span<const uint8_t> payload);

    void setPongHandler(std::function<void(gsl::span<const uint8_t>)> handler);

    /// read next message from the network
    void read(ReadCallbackFunc cb) override;

    /// write the given bytes to the network
    void write(gsl::span<const uint8_t> buffer, WriteCallbackFunc cb) override;

    enum class ReasonOfClose;
    void close(ReasonOfClose reason);

    enum class ReadingState {
      WaitHeader,
      ReadOpcodeAndPrelen,
      ReadSizeAndMask,
      HandleHeader,
      WaitData,
      ReadData,
      HandleData,
      Closed
    };

    enum class Error {
      NOT_ENOUGH_DATA,
      UNEXPECTED_CONTINUE,
      INTERNAL_ERROR,
      CLOSED,
      UNKNOWN_OPCODE
    };

    enum class ReasonOfClose {
      NORMAL_CLOSE,
      TOO_LONG_PING_PAYLOAD,
      TOO_LONG_PONG_PAYLOAD,
      TOO_LONG_CLOSE_PAYLOAD,
      TOO_LONG_DATA_PAYLOAD,
      SOME_DATA_AFTER_CLOSED_BY_REMOTE,
      PINGING_TIMEOUT,

      UNEXPECTED_CONTINUE,
      INTERNAL_ERROR,
      CLOSED,
      UNKNOWN_OPCODE
    };

    enum class Opcode : uint8_t {
      Continue = 0x00,    // continue of previous frame
      Text = 0x01,        // test frame
      Binary = 0x02,      // binary frame
      _data_0x03 = 0x03,  // reserve type of data frame
      _data_0x04 = 0x04,  // reserve type of data frame
      _data_0x05 = 0x05,  // reserve type of data frame
      _data_0x06 = 0x06,  // reserve type of data frame
      _data_0x07 = 0x07,  // reserve type of data frame
      Close = 0x08,       // closing frame
      Ping = 0x09,        // PING
      Pong = 0x0A,        // PONG
      _crtl_0x0B = 0x0B,  // reserve type of control frame
      _crtl_0x0C = 0x0C,  // reserve type of control frame
      _crtl_0x0D = 0x0D,  // reserve type of control frame
      _crtl_0x0E = 0x0E,  // reserve type of control frame
      _crtl_0x0F = 0x0F,  // reserve type of control frame
      _undefined = 0xFF   // dummy type for internal usage
    };

   private:
    template <class CallBack>
    void defferCall(CallBack &&cb) {
      scheduler_->schedule([cb = std::forward<CallBack>(cb)] { cb(); });
    }

    void consume(size_t size);

    void read(size_t size, std::function<void()> cb);

    void readFlagsAndPrelen();
    void handleFlagsAndPrelen();
    void readSizeAndMask();
    void handleSizeAndMask();
    void handleFrame();
    void readData();
    void handleData();

    bool hasOutgoingData();
    void shutdown(outcome::result<void> res);

    enum class ControlFrameType { Ping, Pong, Close };
    void sendControlFrame(ControlFrameType type,
                          std::shared_ptr<common::ByteArray> payload);
    void sendDataFrame();
    void sendData();

    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<connection::LayerConnection> connection_;
    std::function<void(gsl::span<const uint8_t>)> read_pong_handler_;

    std::shared_ptr<common::ByteArray> read_buffer_;
    size_t read_bytes_ = 0;

    struct WritingItem {
      common::ByteArray data;
      WriteCallbackFunc cb;
      size_t header_size;
      size_t written_bytes;
      size_t sent_bytes;
      WritingItem(common::ByteArray data, WriteCallbackFunc cb)
          : data(std::move(data)),
            cb(std::move(cb)),
            header_size(0),
            written_bytes(0),
            sent_bytes(0) {}
    };
    std::deque<WritingItem> writing_queue_;

    ReadCallbackFunc read_data_handler_;

    ReadingState reading_state_ = ReadingState::WaitHeader;
    Opcode last_frame_opcode_ = Opcode::_undefined;
    common::ByteArray incoming_control_data_;
    std::shared_ptr<common::ByteArray> outgoing_ping_data_;
    std::shared_ptr<common::ByteArray> outgoing_pong_data_;
    std::shared_ptr<common::ByteArray> outgoing_close_data_;

    bool closed_by_host_ = false;
    bool closed_by_remote_ = false;

    struct Context {
      bool finally = true;
      Opcode opcode = Opcode::_undefined;
      uint8_t prelen = 0;
      bool masked = false;
      uint8_t mask_index = 0;
      size_t length = 0;
      std::array<uint8_t, 4> mask{0};
      size_t remaining_data = 0;
    } ctx;

    log::Logger log_;
  };

}  // namespace libp2p::connection::websocket

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection::websocket, WsReadWriter::Error);

#endif  // LIBP2P_LAYER_WEBSOCKET_WSREADWRITTER
