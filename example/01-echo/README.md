# Example Libp2p Echo
## General description

This example shows you how to create a simple Echo client and server in cpp-libp2p. There are three ways to launch this example: C++ to C++, C++ Client to Go Server and C++ Server to Go Client. The last two examples can be used to test a compatibility between the implementations and should be moved to some separate repository to produce automatic tests.

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
3. Execute `../../cmake-build-debug/example/01-echo/libp2p_echo_server`. It will start a C++ server.
4. Execute `./libp2p_client -insecure -l 40011 -d /ip4/127.0.0.1/tcp/40010/ipfs/12D3KooWLs7RC93EGXZzn9YdKyZYYx3f9UjTLYNX1reThpCkFb83`
5. Watch how message is exchanged
