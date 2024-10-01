#!/usr/bin/env -S deno run -A --watch

Deno.stdout.writeSync(new TextEncoder().encode("\x1b[2J\x1b[3J\x1b[1;1H"));

import * as scale from "./scale.ts";
import * as types from "./types.ts";

const events = scale.read(Deno.args[0]);
const events_map = new Map(events.map((e) => [e.event_id, e]));

const protocols = new Map<number, string>();
for (const e of events) {
  if (e.v.type !== "Event_Conn") continue;
  if (e.v.v.type !== "Event_Conn_Stream_Protocol") continue;
  protocols.set(e.v.conn_id, e.v.v.proto);
}
