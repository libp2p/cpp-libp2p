/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/read.hpp>
#include <libp2p/basic/varint_reader.hpp>
#include <libp2p/basic/write.hpp>
#include <qtils/varint_encode.hpp>

namespace libp2p {
  /**
   * Read varint size prefixed frame.
   */
  class ReadVarintFrame {
   public:
    using Result = PollOutcome<Void>;
    Result poll(PollWaker waker, basic::Reader &reader, Bytes &buffer) {
      while (true) {
        if (auto *state_varint = std::get_if<basic::VarintReader>(&state_)) {
          auto n = POLL_OK_TRY(state_varint->poll(waker, reader));
          buffer.resize(n);
          state_ = Read{};
        } else {
          auto &state_frame = std::get<Read>(state_);
          POLL_OK_TRY(state_frame.poll(waker, reader, buffer));
          return Result{Void{}};
        }
      }
    }

   private:
    std::variant<basic::VarintReader, Read> state_;
  };

  /**
   * Write varint size prefixed frame.
   */
  class WriteVarintFrame {
   public:
    using Result = PollOutcome<Void>;
    Result poll(PollWaker waker, basic::Writer &writer, BytesIn buffer) {
      while (true) {
        if (auto *state_varint = std::get_if<WriteVarint>(&state_)) {
          qtils::VarintEncode varint{buffer.size()};
          POLL_OK_TRY(state_varint->write.poll(waker, writer, varint.buffer()));
          state_ = Write{};
        } else {
          auto &state_frame = std::get<Write>(state_);
          POLL_OK_TRY(state_frame.poll(waker, writer, buffer));
          return Result{Void{}};
        }
      }
    }

   private:
    struct WriteVarint {
      Write write;
    };
    std::variant<WriteVarint, Write> state_;
  };
}  // namespace libp2p
