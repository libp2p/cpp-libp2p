# Example Libp2p Echo Server

In order to build the C++ Libp2p Echo Server and test its compatibility with the Go Libp2p Echo Client, do the following:

1. Build a C++ target 
2. Build a Go example via `go build libp2p_client.go`
3. Start a C++ server. It will run for some number of seconds
4. Execute `./libp2p_client -insecure -l 40011 -d /ip4/127.0.0.1/tcp/40010/ipfs/12D3KooWLs7RC93EGXZzn9YdKyZYYx3f9UjTLYNX1reThpCkFb83`
5. Watch how "Hello, world!" is exchanged between server and client
