import { generateKeyPairFromSeed } from "@libp2p/crypto/keys";
import * as Interop from "@libp2p/interop";
import { multiaddr } from "@multiformats/multiaddr";
import { createClient } from "@libp2p/daemon-client";
import * as ChildProcess from "node:child_process";
import * as Path from "node:path";
import * as Net from "node:net";

const PROJECT = Path.join(import.meta.dirname, "../../../..");

function hexEncode(a: Uint8Array) {
  return Buffer.from(a).toString("hex");
}

async function delay(ms: number) {
  await new Promise((r) => setTimeout(r, ms));
}

async function waitForTcp(port: number) {
  while (true) {
    const socket = Net.connect(port);
    const ready = await new Promise((r) => {
      socket.on("connect", () => {
        socket.destroy();
        r(true);
      });
      socket.on("error", () => r(false));
    });
    if (ready) break;
    await delay(10);
  }
}

function tcpAddr(port: number) {
  return `/ip4/127.0.0.1/tcp/${port}`;
}

const factory = {
  port: 10000,
  async spawn(options: Interop.SpawnOptions): Promise<Interop.Daemon> {
    const control_port = this.port++;
    const ports = [control_port];
    const control_addr = multiaddr(tcpAddr(control_port));
    const seed = new Uint8Array(32);
    new DataView(seed.buffer).setUint16(0, this.port);
    const keys = await generateKeyPairFromSeed("Ed25519", seed);
    const cmd = [
      ...{
        js: [
          process.execPath,
          Path.join(PROJECT, "interop/js/dist/src/index.js"),
        ],
        cpp: [Path.join(PROJECT, "interop/cpp/interop")],
      }[options.type],
      control_addr.toString(),
      hexEncode(keys.raw),
      options.encryption ?? "plaintext",
    ];
    if (!options.noListen) {
      const listen_port = this.port++;
      cmd.push(tcpAddr(listen_port));
      ports.push(listen_port);
    }
    const child = ChildProcess.spawn(cmd[0], cmd.slice(1), {
      stdio: "inherit",
    });
    const on_exit = new Promise((r) => child.on("exit", r));
    await Promise.race([on_exit, Promise.all(ports.map(waitForTcp))]);
    return {
      client: createClient(control_addr),
      async stop() {
        child.kill();
        await on_exit;
      },
    };
  },
};

Interop.connectInteropTests(factory);
Interop.streamInteropTests(factory);
Interop.dhtInteropTests(factory);
Interop.pubsubInteropTests(factory);
