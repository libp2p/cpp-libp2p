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

    /**
     * Initializes read writer
     * @param connection - raw connection
     * @param buffer - pointer to buffer where to store bytes from network
     */
    WsReadWriter(std::shared_ptr<basic::Scheduler> scheduler,
                 std::shared_ptr<connection::LayerConnection> connection,
                 gsl::span<const uint8_t> preloaded_data);

    /// read next message from the network
    void read(ReadCallbackFunc cb) override;

    /// write the given bytes to the network
    void write(gsl::span<const uint8_t> buffer, WriteCallbackFunc cb) override;

    void sendContinueFrame();
    void sendTextFrame();
    void sendData();
    void sendClose();
    void sendPing(gsl::span<const uint8_t> buffer, WriteCallbackFunc cb);
    void sendPong(gsl::span<const uint8_t> buffer);

    enum class ReadingState {
      WaitHeader,
      ReadOpcodeAndPrelen,
      ReadSizeAndMask,
      HandleHeader,
      WaitData,
      ReadData,
      HandleData
    };
    enum class WritingState {
      Ready,
      CreateHeader,
      SendHeader,
      SendData,
      SendPing,
      SendPong,
      SendClose
    };

    enum class Error {
      NOT_ENOUGH_DATA,
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
    void readSome(size_t size, std::function<void()> cb);

    void readFlagsAndPrelen();
    void handleFlagsAndPrelen();
    void readSizeAndMask();
    void handleSizeAndMask();
    void handleFrame();
    void readData();
    void handleData();

    std::shared_ptr<basic::Scheduler> scheduler_;
    std::shared_ptr<connection::LayerConnection> connection_;

    std::shared_ptr<common::ByteArray> read_buffer_;
    size_t read_bytes_ = 0;

    struct WritingItem {
      common::ByteArray data;
      size_t header_size = 0;
      size_t written_bytes = 0;
      size_t sent_bytes = 0;
      WriteCallbackFunc cb;
    };
    std::deque<WritingItem> writing_queue_;

    ReadCallbackFunc read_cb_;
    ReadCallbackFunc pong_cb_;

    ReadingState reading_state_ = ReadingState::WaitHeader;
    WritingState writing_state_ = WritingState::Ready;
    Opcode last_frame_opcode_;
    std::shared_ptr<common::ByteArray> incoming_ping_data_;
    std::shared_ptr<common::ByteArray> incoming_pong_data_;

    bool closed_by_host_ = false;
    bool closed_by_remote_ = false;

    struct Context {
      bool finally;
      Opcode opcode;
      uint8_t prelen;
      bool masked;
      uint8_t mask_index = 0;
      size_t length = 0;
      std::array<uint8_t, 4> mask;
      size_t remaining_data = 0;
    } ctx;

    log::Logger log_;
  };

}  // namespace libp2p::connection::websocket

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection::websocket, WsReadWriter::Error);

#endif  // LIBP2P_LAYER_WEBSOCKET_WSREADWRITTER
