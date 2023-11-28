#pragma once

#include <libp2p/basic/readwriter.hpp>
#include <libp2p/transport/quic/libp2p-quic-api.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/connection/stream.hpp>
#include <nexus/quic/stream.hpp>

namespace libp2p::peer {
  class PeerId;
}

namespace libp2p::transport
{
  class QuicConnection;
}
namespace libp2p::connection {
    

  class LIBP2P_QUIC_API QuicStream : public libp2p::connection::Stream
  {
    friend class libp2p::transport::QuicConnection;
    public:
    QuicStream( libp2p::transport::QuicConnection& conn ,bool is_initiator );

    ~QuicStream() override = default;

    /**
     * Check, if this stream is closed from this side of the connection and
     * thus cannot be read from
     * @return true, if stream cannot be read from, false otherwise
     */
    virtual bool isClosedForRead() const override;

    /**
     * Check, if this stream is closed from the other side of the connection and
     * thus cannot be written to
     * @return true, if stream cannot be written to, false otherwise
     */
    virtual bool isClosedForWrite() const override;

    /**
     * Check, if this stream is closed bor both writes and reads
     * @return true, if stream is closed entirely, false otherwise
     */
    virtual bool isClosed() const override;

    /**
     * Close a stream, indicating we are not going to write to it anymore; the
     * other side, however, can write to it, if it was not closed from there
     * before
     * @param cb to be called, when the stream is closed, or error happens
     */
    virtual void close( Stream::VoidResultHandlerFunc cb) override;

    /**
     * @brief Close this stream entirely; this normally means an error happened,
     * so it should not be used just to close the stream
     */
    virtual void reset() override;

    /**
     * Set a new receive window size of this stream - how much unread bytes can
     * we have on our side of the stream
     * @param new_size for the window
     * @param cb to be called, when the operation succeeds of fails
     */
    virtual void adjustWindowSize(uint32_t new_size,
                                  libp2p::connection::Stream::VoidResultHandlerFunc cb) override;

    /**
     * Is that stream opened over a connection, which was an initiator?
     */
    virtual outcome::result<bool> isInitiator() const override;

    /**
     * Get a peer, which the stream is connected to
     * @return id of the peer
     */
    virtual outcome::result<peer::PeerId> remotePeerId() const override;

    /**
     * Get a local multiaddress
     * @return address or error
     */
    virtual outcome::result<multi::Multiaddress> localMultiaddr() const override;

    /**
     * Get a multiaddress, to which the stream is connected
     * @return multiaddress or error
     */
    virtual outcome::result<multi::Multiaddress> remoteMultiaddr() const override;

// Reader
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
    virtual void read(BytesOut out, size_t bytes, basic::Reader::ReadCallbackFunc cb) override;

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
    virtual void readSome(BytesOut out, size_t bytes, basic::Reader::ReadCallbackFunc cb) override;

    /**
     * @brief Defers reporting result or error to callback to avoid reentrancy
     * (i.e. callback will not be called before initiator function returns)
     * @param res read result
     * @param cb callback
     */
    virtual void deferReadCallback(outcome::result<size_t> res,
                                   basic::Reader::ReadCallbackFunc cb) override;

    // Writer 


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
    virtual void write(BytesIn in, size_t bytes, basic::Writer::WriteCallbackFunc cb) override;

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
    virtual void writeSome(BytesIn in, size_t bytes, basic::Writer::WriteCallbackFunc cb) override;

    /**
     * @brief Defers reporting error state to callback to avoid reentrancy
     * (i.e. callback will not be called before initiator function returns)
     * @param ec error code
     * @param cb callback
     *
     * @note if (!ec) then this function does nothing
     */
    virtual void deferWriteCallback(std::error_code ec,
                                    basic::Writer::WriteCallbackFunc cb) override;


private:
  bool m_is_initiator;

    nexus::quic::stream m_stream;
    libp2p::transport::QuicConnection& m_conn;
    log::Logger log_;
  };
}  // namespace libp2p::connection
