# Gossip pub-sub examples
## General description

* `gossip_chat_example.cpp` shows how to use Gossip pub-sub
* `utility.cpp` and `console_async_reader.*` contain common things related to examples

## Launching

### gossip_chat_example

`./gossip_chat_example -p PORT [-t TOPIC] [-r REMOTE] [-l LOG_LEVEL]`

Longer strings for options are available, respectively `--port`, `--topic`, `--remote`, and `--log`

* `PORT` : port the local node is going to listen to
* `TOPIC` : topic name for chat messages
* `REMOTE` : p2p URI of bootstrap peer to connect to
* `LOG_LEVEL`: verbosity level of log output, possible values are `d`, `i`, `w`, or `e` (self explained)

After launching, you may type messages into console, which are being published into the topic.
Log messages are printed into`stdout`. Chat output is written to `stderr`. So logs may be redirected if needed

Explanation by example: `./gossip_chat_example --log=d -p 1300 -r /ip4/192.168.0.104/tcp/10000/p2p/12D3KooWLm1CRdj8DBhwAfMGEcNGCEm3ZWxL6jD6uC6BizqXGcP6` means that

* Logs will be printed with `debug` verbosity level
* local node will listen to port 1300 and try to connect to remote peer given by uri
