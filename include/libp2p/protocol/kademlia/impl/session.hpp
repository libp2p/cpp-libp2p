/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_SESSION
#define LIBP2P_PROTOCOL_KADEMLIA_SESSION

#include <libp2p/connection/stream.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/kad/impl/kad_session_host.hpp>

namespace libp2p::protocol::kademlia {

class PeerFinder;

class Session
		: public std::enable_shared_from_this<Session> {
public:
    enum class State {
      closed = 0,
      reading_from_peer,
      writing_to_peer
    };

		Session(std::weak_ptr<PeerFinder> host,
		                   std::shared_ptr<connection::Stream> stream,
		                   scheduler::Ticks operations_timeout=0);

		bool write(const std::vector<uint8_t>& buffer);

		bool read();

		// state is impl defined
		State state() {
			return state_;
		}
		void state(State new_state) {
			state_ = new_state;
		}

		void close();

private:
		void onLengthRead(boost::optional<multi::UVarint> varint_opt);

		void onMessageRead(outcome::result<size_t> res);

		void onMessageWritten(outcome::result<size_t> res);

		void setTimeout();
		void cancelTimeout();

		std::weak_ptr<PeerFinder> host_;
		std::shared_ptr<connection::Stream> stream_;

		// true if msg length is read and waiting for message body
		bool reading_ = false;

		// session-defined state
		State state_ = State::closed;

		const scheduler::Ticks operations_timeout_;
		Scheduler::Handle timeout_handle_;
};

}  // namespace libp2p::protocol::kad

#endif  // LIBP2P_PROTOCOL_KADEMLIA_SESSION
