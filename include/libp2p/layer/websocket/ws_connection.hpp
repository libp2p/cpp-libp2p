/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_WSCONNECTION
#define LIBP2P_CONNECTION_WSCONNECTION

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/layer_connection.hpp>
#include <libp2p/layer/websocket/ws_connection_config.hpp>
#include <libp2p/layer/websocket/ws_read_writer.hpp>
#include <libp2p/log/logger.hpp>

namespace libp2p::connection {

  class WsConnection final : public LayerConnection,
                             public std::enable_shared_from_this<WsConnection> {
   public:
    using BufferList = std::list<common::ByteArray>;

    struct OperationContext {
      size_t bytes_served;                   /// written or read bytes count
      const size_t total_bytes;              /// total size to process
      BufferList::iterator write_buffer_it;  /// temporary data storage
    };

    WsConnection(const WsConnection &other) = delete;
    WsConnection &operator=(const WsConnection &other) = delete;
    WsConnection(WsConnection &&other) = delete;
    WsConnection &operator=(WsConnection &&other) = delete;
    ~WsConnection() override = default;

    /**
     * Create a new WsConnection instance
     * @param connection to be wrapped to websocket by this instance
     */
    explicit WsConnection(
        std::shared_ptr<const layer::WsConnectionConfig> config,
        std::shared_ptr<LayerConnection> connection,
        std::shared_ptr<basic::Scheduler> scheduler,
        gsl::span<const uint8_t> preloaded_data);

    bool isInitiator() const noexcept override;

    void start();
    void stop();

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    outcome::result<void> close() override;

    bool isClosed() const override;

    void read(gsl::span<uint8_t> out, size_t bytes,
              ReadCallbackFunc cb) override;

    void readSome(gsl::span<uint8_t> out, size_t bytes,
                  ReadCallbackFunc cb) override;

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    void write(gsl::span<const uint8_t> in, size_t bytes,
               WriteCallbackFunc cb) override;

    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   WriteCallbackFunc cb) override;

    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

   private:
    using Buffer = common::ByteArray;

    void read(gsl::span<uint8_t> out, size_t bytes, OperationContext ctx,
              ReadCallbackFunc cb);

    void readSome(gsl::span<uint8_t> out, size_t required_bytes,
                  OperationContext ctx, ReadCallbackFunc cb);

    void onPong(gsl::span<const uint8_t> payload);

    /// Config
    std::shared_ptr<const layer::WsConnectionConfig> config_;

    /// Underlying connection
    std::shared_ptr<LayerConnection> connection_;

    /// Scheduler
    std::shared_ptr<basic::Scheduler> scheduler_;

    /// True if started
    bool started_ = false;

    BufferList read_buffers_;
    std::shared_ptr<websocket::WsReadWriter> ws_read_writer_;
    BufferList write_buffers_;
    log::Logger log_ = log::createLogger("WsConnection");

    /// Timer handle for pings
    size_t ping_counter_ = 0u;
    basic::Scheduler::Handle ping_handle_;
    basic::Scheduler::Handle ping_timeout_handle_;

    /// Timer handle for auto closing if inactive
    basic::Scheduler::Handle inactivity_handle_;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(libp2p::connection::WsConnection);
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_CONNECTION_WSCONNECTION
