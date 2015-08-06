This is a fork of the awesome libjson-rpc-cpp library (https://github.com/cinemast/libjson-rpc-cpp). The main target of this fork is to add a WebSocket support in order to establish a bidirectional connection between the server and its clients.

Get the source:

```sh
$ git clone https://github.com/toolmmy/libjson-rpc-cpp.git
```

Build the source:

```sh
$ cd libjson-rpc-cpp/build/
libjson-rpc-cpp/build$ cmake .. && make -j4
-- The C compiler identification is GNU 4.8.2
-- The CXX compiler identification is GNU 4.8.2
-- Check for working C compiler: /usr/bin/cc
[...]
Linking CXX executable ../../../out/simplewebsocketserver
[100%] Built target simplewebsocketserver
```

Start the websocketserver example:

```sh
libjson-rpc-cpp/build$ cd out/
libjson-rpc-cpp/build/out$ export JSONRPC_VERBOSE=1 #optional, enables verbose mode
libjson-rpc-cpp/build/out$ ./simplewebsocketserver 
jsonrpc: [WebsocketServer] Starting listening on port <9876>
jsonrpc: [WebsocketServer] Starting client connection maintenance thread
Server started successfully
```

Open your firefox and load the test page (in a second terminal):
```sh
libjson-rpc-cpp/build/out$ firefox wsclient.html
```

