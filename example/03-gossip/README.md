# Gossip pub-sub examples
## General description

* `gossip_chat_example.cpp` shows how to use Gossip pub-sub (publish, subscribe, lifetime management)
* `gossip_example.cpp` contains non-interactive simulation of intense messaging across multiple hosts and topics 
* `utility.cpp` contains common things related to examples
* `factory.cpp` contains creation code using boost::di (and example how to parametrize boost::di and make different host entities inside a process)

## Launching

### gossip_chat_example

`./gossip_chat_example LOCAL_PORT LOCAL_KEY [REMOTE_HOST REMOTE_PORT REMOTE_KEY] > log.log`

* `LOCAL_PORT` : port the local node is going to listen to
* `LOCAL_KEY` : some shortcut key string from which local peer id will de derived (see explanation below)
* `REMOTE_HOST` : ip4 address of host to be added as boostrap peer host to gossip engine
* `REMOTE_PORT` : port on `REMOTE_HOST` to connect to
* `REMOTE_KEY` : shortcut key of bootstrap peer
* `> log.log` is used to redirect `stdout` (which contain extensive debug logs). Chat output is written to `stderr`

After launching, you may type messages into console, which are being published into "chat" topic
Also, there is another topic "peers" which serves as rendezvous, i.e. peers publish their listen addresses into that topic to allow connectivity to grow

Explanation by example: `./gossip_chat_example 13000 pex 192.168.0.104 10000 fex` means that

* local node will listen to port 13000
* the word `pex` will be used to emit ed25519 private key (as sha256(pex)) and derive the public key and peer id from it. Just see this trick in `utility.cpp` and don't use such an approach anywhere besides tests, :)
* local node will connect to 192.168.0.104:10000 and peer id derived from `fex` magic word

`./gossip_chat_example LOCAL_PORT LOCAL_KEY` without `REMOTE_*` part will just listen to incoming connections on port `LOCAL_PORT`
     
## gossip_example

`./gossip_example [nHosts] [debugLog]` launches continuous message exchange in a single process which creates `nHosts` internal peers, non-zero parameter `debugLog` leads to output debug log messages as well
