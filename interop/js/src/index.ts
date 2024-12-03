import { privateKeyFromRaw } from "@libp2p/crypto/keys";
import { multiaddr } from "@multiformats/multiaddr";
import { createServer } from "@libp2p/daemon-server";
import { createLibp2p } from "libp2p";
import { tcp } from "@libp2p/tcp";
import { identify } from "@libp2p/identify";
import { kadDHT } from "@libp2p/kad-dht";
import { gossipsub } from "@chainsafe/libp2p-gossipsub";
import { noise } from "@chainsafe/libp2p-noise";
import { yamux } from "@chainsafe/libp2p-yamux";
import { ping } from "@libp2p/ping";
import { plaintext } from "@libp2p/plaintext";

function hexDecode(s: string) {
  return Buffer.from(s, "hex");
}

export async function main(args: string[]) {
  const [arg_control, arg_key, arg_encryption, ...arg_listen] = args;
  const node = await createLibp2p({
    privateKey: privateKeyFromRaw(hexDecode(arg_key)),
    addresses: { listen: arg_listen },
    transports: [tcp()],
    connectionEncrypters: [
      { plaintext: plaintext(), noise: noise() }[arg_encryption]!,
    ],
    streamMuxers: [yamux()],
    services: {
      identify: identify(),
      ping: ping(),
      dht: kadDHT({
        clientMode: false,
        peerInfoMapper(info) {
          return info;
        },
      }),
      pubsub: gossipsub(),
    },
  });
  const server = createServer(multiaddr(arg_control), node as any);
  await server.start();
  await new Promise(() => {});
}

main(process.argv.slice(2)).catch(console.error).then(() => process.exit());
