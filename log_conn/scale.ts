import * as types from "./types.ts";

const utf8 = new TextDecoder();
export class Scale {
  pos = 0;
  view: DataView;
  constructor(public buf: Uint8Array) {
    this.view = new DataView(buf.buffer, buf.byteOffset, buf.length);
  }
  get more() {
    return this.buf.length - this.pos;
  }
  u8() {
    return this.buf[this.skip(1)];
  }
  u16() {
    return this.view.getUint16(this.skip(2), true);
  }
  u32() {
    return this.view.getUint32(this.skip(4), true);
  }
  u() {
    const m = this.buf[this.pos] & 3;
    if (m === 0) {
      return this.u8() >> 2;
    }
    if (m === 1) {
      return this.u16() >> 2;
    }
    if (m === 2) {
      return this.u32() >> 2;
    }
    const n = 4 + (this.u8() >> 2);
    let r = 0, s = 1;
    for (let i = 0; i < n; ++i) {
      r += this.u8() * s;
      s *= 256;
    }
    return r;
  }
  bool() {
    return this.u8() !== 0;
  }
  opt<T>(f: (s: Scale) => T) {
    const n = this.u8();
    if (n === 0) {
      return null;
    }
    if (n !== 1) {
      throw new Error();
    }
    return f(this);
  }
  read(n: number) {
    const pos = this.skip(n);
    return this.buf.subarray(pos, pos + n);
  }
  skip(n: number) {
    const { pos } = this;
    if (pos + n > this.buf.length) {
      throw new Error();
    }
    this.pos += n;
    return pos;
  }
  bytes() {
    return this.read(this.u());
  }
  str() {
    return utf8.decode(this.bytes());
  }
}

export function Event_CbLost(s: Scale): types.Event_CbLost {
  return { type: "Event_CbLost", call_id: s.u() };
}
export function Event_CbMuch(s: Scale): types.Event_CbMuch {
  return { type: "Event_CbMuch", call_id: s.u() };
}
export function Event_Conn_Dns(_s: Scale): types.Event_Conn_Dns {
  return { type: "Event_Conn_Dns" };
}
export function Event_Conn_Dns_Cb(s: Scale): types.Event_Conn_Dns_Cb {
  return { type: "Event_Conn_Dns_Cb", call_id: s.u(), ok: s.bool() };
}
export function Event_Conn_Tcp_Dtor(_s: Scale): types.Event_Conn_Tcp_Dtor {
  return { type: "Event_Conn_Tcp_Dtor" };
}
export function Event_Conn_Tcp_Close(_s: Scale): types.Event_Conn_Tcp_Close {
  return { type: "Event_Conn_Tcp_Close" };
}
export function Event_Conn_Tcp_Dial(s: Scale): types.Event_Conn_Tcp_Dial {
  return { type: "Event_Conn_Tcp_Dial", addr: s.str() };
}
export function Event_Conn_Tcp_Connect(
  _s: Scale,
): types.Event_Conn_Tcp_Connect {
  return { type: "Event_Conn_Tcp_Connect" };
}
export function Event_Conn_Tcp_Connect_Cb(
  s: Scale,
): types.Event_Conn_Tcp_Connect_Cb {
  return { type: "Event_Conn_Tcp_Connect_Cb", call_id: s.u(), ok: s.bool() };
}
export function Event_Conn_Tcp_Connect_Timeout(
  _s: Scale,
): types.Event_Conn_Tcp_Connect_Timeout {
  return { type: "Event_Conn_Tcp_Connect_Timeout" };
}
export function Event_Conn_Tcp_Accept(
  s: Scale,
): types.Event_Conn_Tcp_Accept {
  return { type: "Event_Conn_Tcp_Accept", addr: s.str() };
}
export function Event_Conn_Ssl(s: Scale): types.Event_Conn_Ssl {
  return { type: "Event_Conn_Ssl", parent_id: s.u() };
}
export function Event_Conn_Ssl_Dtor(_s: Scale): types.Event_Conn_Ssl_Dtor {
  return { type: "Event_Conn_Ssl_Dtor" };
}
export function Event_Conn_Ssl_Close(_s: Scale): types.Event_Conn_Ssl_Close {
  return { type: "Event_Conn_Ssl_Close" };
}
export function Event_Conn_Ssl_Cb(s: Scale): types.Event_Conn_Ssl_Cb {
  return { type: "Event_Conn_Ssl_Cb", call_id: s.u(), ok: s.bool() };
}
export function Event_Conn_Ws(s: Scale): types.Event_Conn_Ws {
  return { type: "Event_Conn_Ws", parent_id: s.u() };
}
export function Event_Conn_Ws_Dtor(_s: Scale): types.Event_Conn_Ws_Dtor {
  return { type: "Event_Conn_Ws_Dtor" };
}
export function Event_Conn_Ws_Close(_s: Scale): types.Event_Conn_Ws_Close {
  return { type: "Event_Conn_Ws_Close" };
}
export function Event_Conn_Ws_Cb(s: Scale): types.Event_Conn_Ws_Cb {
  return { type: "Event_Conn_Ws_Cb", call_id: s.u(), ok: s.bool() };
}
export function Event_Conn_Noise(s: Scale): types.Event_Conn_Noise {
  return { type: "Event_Conn_Noise", parent_id: s.u() };
}
export function Event_Conn_Noise_Dtor(_s: Scale): types.Event_Conn_Noise_Dtor {
  return { type: "Event_Conn_Noise_Dtor" };
}
export function Event_Conn_Noise_Close(
  _s: Scale,
): types.Event_Conn_Noise_Close {
  return { type: "Event_Conn_Noise_Close" };
}
export function Event_Conn_Noise_Mismatch(
  _s: Scale,
): types.Event_Conn_Noise_Mismatch {
  return { type: "Event_Conn_Noise_Mismatch" };
}
export function Event_Conn_Noise_Cb(s: Scale): types.Event_Conn_Noise_Cb {
  return {
    type: "Event_Conn_Noise_Cb",
    call_id: s.u(),
    peer: s.opt(() => s.str()),
  };
}
export function Event_Conn_Yamux(s: Scale): types.Event_Conn_Yamux {
  return { type: "Event_Conn_Yamux", parent_id: s.u() };
}
export function Event_Conn_Yamux_Dtor(_s: Scale): types.Event_Conn_Yamux_Dtor {
  return { type: "Event_Conn_Yamux_Dtor" };
}
export function Event_Conn_Yamux_Close(
  _s: Scale,
): types.Event_Conn_Yamux_Close {
  return { type: "Event_Conn_Yamux_Close" };
}
export function Event_Conn_Stream(s: Scale): types.Event_Conn_Stream {
  return { type: "Event_Conn_Stream", parent_id: s.u(), out: s.bool() };
}
export function Event_Conn_Stream_Dtor(
  _s: Scale,
): types.Event_Conn_Stream_Dtor {
  return { type: "Event_Conn_Stream_Dtor" };
}
export function Event_Conn_Stream_Close(
  _s: Scale,
): types.Event_Conn_Stream_Close {
  return { type: "Event_Conn_Stream_Close" };
}
export function Event_Conn_Stream_Reset(
  _s: Scale,
): types.Event_Conn_Stream_Reset {
  return { type: "Event_Conn_Stream_Reset" };
}
export function Event_Conn_Stream_Protocol(
  s: Scale,
): types.Event_Conn_Stream_Protocol {
  return { type: "Event_Conn_Stream_Protocol", proto: s.str() };
}
export function Event_Conn_Read(s: Scale): types.Event_Conn_Read {
  return { type: "Event_Conn_Read", n: s.u() };
}
export function Event_Conn_Read_Cb(s: Scale): types.Event_Conn_Read_Cb {
  return {
    type: "Event_Conn_Read_Cb",
    call_id: s.u(),
    buf: s.opt((s) => s.bytes()),
  };
}
export function Event_Conn_Read_CbSize(s: Scale): types.Event_Conn_Read_CbSize {
  return {
    type: "Event_Conn_Read_CbSize",
    call_id: s.u(),
    n: s.opt((s) => s.u()),
  };
}
export function Event_Conn_Write(s: Scale): types.Event_Conn_Write {
  return { type: "Event_Conn_Write", buf: s.bytes() };
}
export function Event_Conn_Write_Cb(s: Scale): types.Event_Conn_Write_Cb {
  return {
    type: "Event_Conn_Write_Cb",
    call_id: s.u(),
    n: s.opt((s) => s.u()),
  };
}
export function Event_Conn_WriteSize(s: Scale): types.Event_Conn_WriteSize {
  return { type: "Event_Conn_WriteSize", n: s.u() };
}
export function Event_Conn(s: Scale): types.Event_Conn {
  return {
    type: "Event_Conn",
    conn_id: s.u(),
    v: [
      Event_Conn_Dns,
      Event_Conn_Dns_Cb,
      Event_Conn_Tcp_Dial,
      Event_Conn_Tcp_Dtor,
      Event_Conn_Tcp_Close,
      Event_Conn_Tcp_Connect,
      Event_Conn_Tcp_Connect_Cb,
      Event_Conn_Tcp_Connect_Timeout,
      Event_Conn_Tcp_Accept,
      Event_Conn_Ssl,
      Event_Conn_Ssl_Dtor,
      Event_Conn_Ssl_Close,
      Event_Conn_Ssl_Cb,
      Event_Conn_Ws,
      Event_Conn_Ws_Dtor,
      Event_Conn_Ws_Close,
      Event_Conn_Ws_Cb,
      Event_Conn_Noise,
      Event_Conn_Noise_Dtor,
      Event_Conn_Noise_Close,
      Event_Conn_Noise_Mismatch,
      Event_Conn_Noise_Cb,
      Event_Conn_Yamux,
      Event_Conn_Yamux_Dtor,
      Event_Conn_Yamux_Close,
      Event_Conn_Stream,
      Event_Conn_Stream_Dtor,
      Event_Conn_Stream_Close,
      Event_Conn_Stream_Reset,
      Event_Conn_Stream_Protocol,
      Event_Conn_Read,
      Event_Conn_Read_Cb,
      Event_Conn_Read_CbSize,
      Event_Conn_Write,
      Event_Conn_Write_Cb,
      Event_Conn_WriteSize,
    ][s.u8()](s),
  };
}
export function Event(s: Scale): types.Event {
  return {
    time: s.u(),
    event_id: s.u(),
    v: [Event_CbLost, Event_CbMuch, Event_Conn][s.u8()](s),
  };
}

export function read(path: string): types.Event[] {
  const events: types.Event[] = [];
  const s = new Scale(Deno.readFileSync(path));
  while (s.more) {
    let a: Uint8Array;
    try {
      a = s.bytes();
    } catch {
      break;
    }
    events.push(Event(new Scale(a)));
  }
  return events;
}
