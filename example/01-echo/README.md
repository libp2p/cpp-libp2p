# Example Libp2p Echo

## General description

This example shows you how to create a simple Echo client and server in cpp-libp2p. 
There are three ways to launch this example: C++ to C++, C++ Client to Go Server and C++ Server to Go Client. 
The last two examples can be used to test a compatibility between the implementations and should be moved to some separate repository to produce automatic tests.


## C++ Server and Client

Currently, `libp2p_echo_server` can operate in two modes - via Plaintext or Noise security protocols.
By default, it is launched in Noise secured mode.
To run it with Plaintext exclusive mode just add `-insecure` command-line argument.

```bash
libp2p_echo_server -insecure
```

The client application does **not** have such an option and supports both types of connection at any time.
Which security type is going to be used is defined by the server's working mode and its preferences.

## Golang Implementation Compatibility Note

The `libp2p_client.go` which is an echo server source from `go-libp2p-examples` can be built only when `go.mod` file is present.
The current version of the `go.mod` is outdated, and only Plaintext security could be cross-tested.
To update the `go.mod` file there are changes required in c++ implementation of libp2p.
Those changes are the subject of ongoin work.

## C++ to C++

1. Build C++ targets `libp2p_echo_server` and `libp2p_echo_client`
2. Start a C++ server. If you build using CMake and now are in this directory, executable can be launched as `../../cmake-build-debug/example/01-echo/libp2p_echo_server`.
4. Start a C++ client. If you build using CMake and now are in directory, executable can be launched as `../../cmake-build-debug/example/01-echo/libp2p_echo_client /ip4/127.0.0.1/tcp/40010/ipfs/12D3KooWLs7RC93EGXZzn9YdKyZYYx3f9UjTLYNX1reThpCkFb83`.
3. Watch how the message is exchanged between server and client
       
## C++ Client to Go Server

1. Build C++ target `libp2p_echo_client`
2. Build a Go example via `go build libp2p_client.go`
3. Execute `./libp2p_client -insecure -l 40010`. It will start a Go server
4. Go server outputs its address in form `I am /ip4/.../tcp/.../ipfs/...`. Please, take a note of this address
5. Start C++ client. If you build using CMake and now are in directory, executable can be launched as `../../cmake-build-debug/example/01-echo/libp2p_echo_client $ADDRESS_OF_GO_SERVER$`.
6. Watch how message is exchanged

## C++ Server and Go Client

1. Build C++ target `libp2p_echo_server`
2. Build a Go example via `go build libp2p_client.go`
3. Execute `../../cmake-build-debug/example/01-echo/libp2p_echo_server -insecure`. It will start a C++ server.
4. Execute `./libp2p_client -insecure -l 40011 -d /ip4/127.0.0.1/tcp/40010/ipfs/12D3KooWLs7RC93EGXZzn9YdKyZYYx3f9UjTLYNX1reThpCkFb83`
5. Watch how message is exchanged
