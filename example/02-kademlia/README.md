# Example using Kademlia for peer discovery

## General description

* `rendezvous_chat.cpp` is a simple chat-client with peer discovery in DHT

## Launching

### rendezvous_chat

```bash
./rendezvous_chat [LISTEN_ADDR]
```

where `LISTEN_ADDR` is libp2p address for incoming connection

Explanation by example: 
`./rendezvous_chat /ip4/127.0.0.1/tcp/3333` means that local node will listen port 3333 on IP 127.0.0.1

### What is happening after launching
1. App connects to bootstrap nodes and starts random-walk to collect peer infos
2. The network (known peers) is notified about owns providing of chat features
3. The app starts searching for other providers with the same chat protocol
4. The app connects to the found peers and accepts incoming connections

### Normal work
First message in terminal contains own peer_id:
```log
12D3KooWKpQQojSESeYK5wgxhiHFfWGt1DT37WMks4e2wXjjTrZj * started
```

Incoming and outgoing connections look like:
```log
12D3KooWNrhWQqbFkwg3Viz3HL2jCdvK9Q3Nieu9QXX37c17Aw2Y + outgoing stream to /ip4/127.0.0.1/tcp/3333
12D3KooWN4U92XG49kSjJ6p5TUEWxj76V432HfbmQAXwryLApkue + incoming stream from /ip4/127.0.0.1/tcp/58442
```

After connection was established you can start chatting by typing a message in terminal:
```log
Hello! Where are you from?
12D3KooWN4U92XG49kSjJ6p5TUEWxj76V432HfbmQAXwryLApkue < Hello! Where are you from?
12D3KooWN4U92XG49kSjJ6p5TUEWxj76V432HfbmQAXwryLApkue > Hi! I'm from Mars.
```

### Note
Finding partner can take some time. You can start two clients on the different ports (or IPs), but it is important that clients can connect to each other.
