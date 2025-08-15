# SwiftWire library

SwiftWire is a small, high‑performance C++ networking library built on Boost.Asio.

It provides a single-thread friendly async client and a scalable async server with a simple framed binary protocol:

- Frame: [4B big‑endian length] + [body]
- HELLO request: [1B type=0x01][8B client_id]
- HELLO_ACK response: [1B type=0x81][8B client_id][1B status]

## Build and use

- Configure and build:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

- Run server example:

```bash
./examples/server_example 0.0.0.0 9000
```

- Run client example:

```bash
./examples/client_example 127.0.0.1 9000 12345
```