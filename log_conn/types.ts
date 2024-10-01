export function cmp<T>(l: T, r: T) {
  return l < r ? -1 : l > r ? 1 : 0;
}

export interface Event_CbLost {
  type: "Event_CbLost";
  call_id: number;
}
export interface Event_CbMuch {
  type: "Event_CbMuch";
  call_id: number;
}
export interface Event_Conn_Dns {
  type: "Event_Conn_Dns";
}
export interface Event_Conn_Dns_Cb {
  type: "Event_Conn_Dns_Cb";
  call_id: number;
  ok: boolean;
}
export interface Event_Conn_Tcp_Dtor {
  type: "Event_Conn_Tcp_Dtor";
}
export interface Event_Conn_Tcp_Close {
  type: "Event_Conn_Tcp_Close";
}
export interface Event_Conn_Tcp_Dial {
  type: "Event_Conn_Tcp_Dial";
  addr: string;
}
export interface Event_Conn_Tcp_Connect {
  type: "Event_Conn_Tcp_Connect";
}
export interface Event_Conn_Tcp_Connect_Cb {
  type: "Event_Conn_Tcp_Connect_Cb";
  call_id: number;
  ok: boolean;
}
export interface Event_Conn_Tcp_Connect_Timeout {
  type: "Event_Conn_Tcp_Connect_Timeout";
}
export interface Event_Conn_Tcp_Accept {
  type: "Event_Conn_Tcp_Accept";
  addr: string;
}
export interface Event_Conn_Ssl {
  type: "Event_Conn_Ssl";
  parent_id: number;
}
export interface Event_Conn_Ssl_Dtor {
  type: "Event_Conn_Ssl_Dtor";
}
export interface Event_Conn_Ssl_Close {
  type: "Event_Conn_Ssl_Close";
}
export interface Event_Conn_Ssl_Cb {
  type: "Event_Conn_Ssl_Cb";
  call_id: number;
  ok: boolean;
}
export interface Event_Conn_Ws {
  type: "Event_Conn_Ws";
  parent_id: number;
}
export interface Event_Conn_Ws_Dtor {
  type: "Event_Conn_Ws_Dtor";
}
export interface Event_Conn_Ws_Close {
  type: "Event_Conn_Ws_Close";
}
export interface Event_Conn_Ws_Cb {
  type: "Event_Conn_Ws_Cb";
  call_id: number;
  ok: boolean;
}
export interface Event_Conn_Noise {
  type: "Event_Conn_Noise";
  parent_id: number;
}
export interface Event_Conn_Noise_Dtor {
  type: "Event_Conn_Noise_Dtor";
}
export interface Event_Conn_Noise_Close {
  type: "Event_Conn_Noise_Close";
}
export interface Event_Conn_Noise_Mismatch {
  type: "Event_Conn_Noise_Mismatch";
}
export interface Event_Conn_Noise_Cb {
  type: "Event_Conn_Noise_Cb";
  call_id: number;
  peer: string | null;
}
export interface Event_Conn_Yamux {
  type: "Event_Conn_Yamux";
  parent_id: number;
}
export interface Event_Conn_Yamux_Dtor {
  type: "Event_Conn_Yamux_Dtor";
}
export interface Event_Conn_Yamux_Close {
  type: "Event_Conn_Yamux_Close";
}
export interface Event_Conn_Stream {
  type: "Event_Conn_Stream";
  parent_id: number;
  out: boolean;
}
export interface Event_Conn_Stream_Dtor {
  type: "Event_Conn_Stream_Dtor";
}
export interface Event_Conn_Stream_Close {
  type: "Event_Conn_Stream_Close";
}
export interface Event_Conn_Stream_Reset {
  type: "Event_Conn_Stream_Reset";
}
export interface Event_Conn_Stream_Protocol {
  type: "Event_Conn_Stream_Protocol";
  proto: string;
}
export interface Event_Conn_Read {
  type: "Event_Conn_Read";
  n: number;
}
export interface Event_Conn_Read_Cb {
  type: "Event_Conn_Read_Cb";
  call_id: number;
  buf: Uint8Array | null;
}
export interface Event_Conn_Read_CbSize {
  type: "Event_Conn_Read_CbSize";
  call_id: number;
  n: number | null;
}
export interface Event_Conn_Write {
  type: "Event_Conn_Write";
  buf: Uint8Array;
}
export interface Event_Conn_Write_Cb {
  type: "Event_Conn_Write_Cb";
  call_id: number;
  n: number | null;
}
export interface Event_Conn_WriteSize {
  type: "Event_Conn_WriteSize";
  n: number;
}
export interface Event_Conn {
  type: "Event_Conn";
  conn_id: number;
  v:
    | Event_Conn_Dns
    | Event_Conn_Dns_Cb
    | Event_Conn_Tcp_Dial
    | Event_Conn_Tcp_Dtor
    | Event_Conn_Tcp_Close
    | Event_Conn_Tcp_Connect
    | Event_Conn_Tcp_Connect_Cb
    | Event_Conn_Tcp_Connect_Timeout
    | Event_Conn_Tcp_Accept
    | Event_Conn_Ssl
    | Event_Conn_Ssl_Dtor
    | Event_Conn_Ssl_Close
    | Event_Conn_Ssl_Cb
    | Event_Conn_Ws
    | Event_Conn_Ws_Dtor
    | Event_Conn_Ws_Close
    | Event_Conn_Ws_Cb
    | Event_Conn_Noise
    | Event_Conn_Noise_Dtor
    | Event_Conn_Noise_Close
    | Event_Conn_Noise_Mismatch
    | Event_Conn_Noise_Cb
    | Event_Conn_Yamux
    | Event_Conn_Yamux_Dtor
    | Event_Conn_Yamux_Close
    | Event_Conn_Stream
    | Event_Conn_Stream_Dtor
    | Event_Conn_Stream_Close
    | Event_Conn_Stream_Reset
    | Event_Conn_Stream_Protocol
    | Event_Conn_Read
    | Event_Conn_Read_Cb
    | Event_Conn_Read_CbSize
    | Event_Conn_Write
    | Event_Conn_Write_Cb
    | Event_Conn_WriteSize;
}
export interface Event {
  time: number;
  event_id: number;
  v: Event_CbLost | Event_CbMuch | Event_Conn;
}

export type Event_Conn_keys = Event_Conn["v"]["type"];
export interface Event_Conn_types {
  Event_Conn_Dns: Event_Conn_Dns;
  Event_Conn_Dns_Cb: Event_Conn_Dns_Cb;
  Event_Conn_Tcp_Dial: Event_Conn_Tcp_Dial;
  Event_Conn_Tcp_Dtor: Event_Conn_Tcp_Dtor;
  Event_Conn_Tcp_Close: Event_Conn_Tcp_Close;
  Event_Conn_Tcp_Connect: Event_Conn_Tcp_Connect;
  Event_Conn_Tcp_Connect_Cb: Event_Conn_Tcp_Connect_Cb;
  Event_Conn_Tcp_Connect_Timeout: Event_Conn_Tcp_Connect_Timeout;
  Event_Conn_Tcp_Accept: Event_Conn_Tcp_Accept;
  Event_Conn_Ssl: Event_Conn_Ssl;
  Event_Conn_Ssl_Dtor: Event_Conn_Ssl_Dtor;
  Event_Conn_Ssl_Close: Event_Conn_Ssl_Close;
  Event_Conn_Ssl_Cb: Event_Conn_Ssl_Cb;
  Event_Conn_Ws: Event_Conn_Ws;
  Event_Conn_Ws_Dtor: Event_Conn_Ws_Dtor;
  Event_Conn_Ws_Close: Event_Conn_Ws_Close;
  Event_Conn_Ws_Cb: Event_Conn_Ws_Cb;
  Event_Conn_Noise: Event_Conn_Noise;
  Event_Conn_Noise_Dtor: Event_Conn_Noise_Dtor;
  Event_Conn_Noise_Close: Event_Conn_Noise_Close;
  Event_Conn_Noise_Mismatch: Event_Conn_Noise_Mismatch;
  Event_Conn_Noise_Cb: Event_Conn_Noise_Cb;
  Event_Conn_Yamux: Event_Conn_Yamux;
  Event_Conn_Yamux_Dtor: Event_Conn_Yamux_Dtor;
  Event_Conn_Yamux_Close: Event_Conn_Yamux_Close;
  Event_Conn_Stream: Event_Conn_Stream;
  Event_Conn_Stream_Dtor: Event_Conn_Stream_Dtor;
  Event_Conn_Stream_Close: Event_Conn_Stream_Close;
  Event_Conn_Stream_Reset: Event_Conn_Stream_Reset;
  Event_Conn_Stream_Protocol: Event_Conn_Stream_Protocol;
  Event_Conn_Read: Event_Conn_Read;
  Event_Conn_Read_Cb: Event_Conn_Read_Cb;
  Event_Conn_Read_CbSize: Event_Conn_Read_CbSize;
  Event_Conn_Write: Event_Conn_Write;
  Event_Conn_Write_Cb: Event_Conn_Write_Cb;
  Event_Conn_WriteSize: Event_Conn_WriteSize;
}
