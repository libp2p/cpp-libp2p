# Example using Kademlia for peer discovery

## General description

* `rendezvous_chat.cpp` is a simple chat-client with peer discovery in DHT

## Launching

### rendezvous_chat

`./rendezvous_chat [LISTEN_ADDR]`

where `LISTEN_ADDR` is libp2p address for incoming connection

Explanation by example: `./rendezvous_chat /ip4/127.0.0.1/tcp/3333` means that

* local node will listen to port 3333

### What is happening after launching
1. app connects to botstrap nodes and start random-walking mechanism to collect peers info
1. noticing network (known peers) about owns providing of chat features
1. inding others providers with the same chat features
1. connecting to found peers and acception connections of others

### Normal work
First message in terminal contains own peer_id:
```log
12D3KooWKpQQojSESeYK5wgxhiHFfWGt1DT37WMks4e2wXjjTrZj * started
```

Incoming and outgoing connections looks like:
```log
12D3KooWNrhWQqbFkwg3Viz3HL2jCdvK9Q3Nieu9QXX37c17Aw2Y + outgoing stream to /ip4/127.0.0.1/tcp/3333
12D3KooWN4U92XG49kSjJ6p5TUEWxj76V432HfbmQAXwryLApkue + incoming stream from /ip4/127.0.0.1/tcp/58442
```

After connection was established you cant start chatting by type message in terminal:
```log
Hello! Are you from?
12D3KooWN4U92XG49kSjJ6p5TUEWxj76V432HfbmQAXwryLApkue < Hello! Are you from?
12D3KooWN4U92XG49kSjJ6p5TUEWxj76V432HfbmQAXwryLApkue > Hi! I'm from Soramitsu.
```
