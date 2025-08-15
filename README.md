# SwiftWire

SwiftWire is a lightweight, high‑performance C++ networking library built on **Boost.Asio**.

It provides:

- **Async TCP client** — single‑thread friendly, with deadline‑aware connect and handshake
- **Async TCP server** — scalable, configurable thread pool, per‑connection write queue
- **Simple binary framing protocol**:
  - Frame: `[4B big‑endian length] + [body]`
  - HELLO request: `[1B type=0x01][8B client_id]`
  - HELLO_ACK: `[1B type=0x81][8B client_id][1B status]`

---

## 📂 Project structure

```bash
swiftwire/
├─ CMakeLists.txt
├─ include/swiftwire/
│ ├─ protocol.hpp
│ ├─ client.hpp
│ └─ server.hpp
├─ src/
│ ├─ client.cpp
│ └─ server.cpp
└─ examples/
├─ CMakeLists.txt
├─ client_example.cpp
└─ server_example.cpp
```

## 🚀 Getting started

### Requirements

- **C++20** compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake** ≥ 3.16
- **Boost** ≥ 1.74 (`system`, `asio`)

### Build

```bash
git clone https://github.com/yourname/swiftwire.git
cd swiftwire
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

-----------------------------

## 🖥️ Running the examples

### Start the server

```bash
./examples/server_example 0.0.0.0 9000
```

### Run the client

```bash
./examples/client_example 127.0.0.1 9000 12345
```

### Expected output:

```bash
Connected.
HELLO_ACK: id=12345 status=0
```

-----------------------------

## ⚡ Quickstart usage

### Client:

```cpp
#include "swiftwire/client.hpp"
#include <boost/asio.hpp>
#include <iostream>

int main() {
    boost::asio::io_context io;
    auto client = std::make_shared<swiftwire::AsyncClient>(io);

    client->async_connect("127.0.0.1", "9000", std::chrono::seconds(5),
        [client](auto ec) {
            if (ec) return std::cerr << "Connect failed: " << ec.message() << "\n";
            client->async_handshake(42, std::chrono::seconds(5),
                [](auto ec2, uint64_t id, uint8_t status) {
                    if (ec2) std::cerr << "Handshake failed: " << ec2.message() << "\n";
                    else std::cout << "HELLO_ACK: id=" << id << " status=" << int(status) << "\n";
                });
        });

    io.run();
}
```

### Server:

```cpp
#include "swiftwire/server.hpp"
#include <boost/asio.hpp>

int main() {
    boost::asio::io_context io;
    swiftwire::ServerConfig cfg;
    swiftwire::AsyncServer server(io, {boost::asio::ip::make_address("0.0.0.0"), 9000}, cfg);
    server.run();
    io.run();
}
```

## 🗺 Architecture overview

```bash
 ┌────────────┐
 │  Client    │
 └─────┬──────┘
       │ async_connect + handshake
       ▼
 ┌────────────┐
 │ TCP Socket │
 └─────┬──────┘
       │ framed binary protocol
       ▼
 ┌────────────┐
 │  Server    │
 │ (Async IO) │
 └────────────┘
```

- Each frame: [length:4B][body]
- Server spawns multiple I/O threads based on ServerConfig
- Client supports connection & handshake deadlines
- Backpressure is applied via write queue limits

## 🧭 Architecture flow diagram

### High‑level data flow

```mermaid
flowchart LR
  subgraph Client["Client (AsyncClient)"]
    C1["Resolve + async_connect()<br/>Deadline via steady_timer"]
    C2["async_handshake(id)<br/>Frame: [len][type=0x01][id]"]
  end

  subgraph Wire["TCP"]
    W["Socket stream"]
  end

  subgraph Server["Server (AsyncServer)"]
    A["Acceptor<br/>async_accept()"]
    subgraph Sess["Session (per-connection)"]
      R1["Read 4B length"]
      R2["Read body"]
      D["Dispatch message<br/>(HELLO -> HELLO_ACK)"]
      Q["Write queue + backpressure<br/>(max bytes)"]
      T["Idle timeout<br/>(steady_timer)"]
    end
  end

  C1 -->|connected| C2 --> W --> A --> R1 --> R2 --> D --> Q --> W --> C2
  T -. monitors .- R1
  T -. monitors .- R2
  Q -.->|async_write| W
  style T fill:#fff2cc,stroke:#d6b656
  style Q fill:#e1f5fe,stroke:#4fc3f7
  style D fill:#e8f5e9,stroke:#66bb6a
```

### Handshake sequence

```mermaid
sequenceDiagram
  autonumber
  participant AC as AsyncClient
  participant NET as TCP
  participant AS as AsyncServer
  participant S as Session

  AC->>NET: async_connect(host,port) + deadline timer
  NET->>AS: TCP connect
  AS->>S: create Session and start()
  AC->>S: [len=9][type=0x01][client_id]
  Note right of S: Read len -> Read body
  S-->>AC: [len=10][type=0x81][client_id][status=0]
  Note over AC,S: Length-prefixed framed messages
  rect rgba(255,242,204,0.5)
    AC-->>AC: connect/handshake timer may cancel ops
    S-->>S: idle timer resets on read/write
  end
```

### Frame formats

```mermaid
flowchart TB
  subgraph HELLO["HELLO (Client -> Server)"]
    H1["[4B length = 9]"]
    H2["[1B type = 0x01]"]
    H3["[8B client_id (big-endian)]"]
  end

  subgraph ACK["HELLO_ACK (Server -> Client)"]
    A1["[4B length = 10]"]
    A2["[1B type = 0x81]"]
    A3["[8B client_id (echo)]"]
    A4["[1B status]"]
  end

  H1 --> H2 --> H3
  A1 --> A2 --> A3 --> A4
```

## ⚙️ Configuration

Server settings in ServerConfig (in server.hpp):

| Field                  | Description                           | Default           |
|------------------------|---------------------------------------|-------------------|
| threads                | I/O worker threads                    | HW concurrency    |
| idle_timeout           | Disconnect after inactivity           | 60s               |
| max_frame              | Max incoming frame size               | 1 MiB             |
| max_write_queue_bytes  | Per-connection write backlog limit    | 8 MiB             |
| tcp_nodelay            | Disable Nagle’s algorithm              | true             |


## 📜 License
MIT — see LICENSE

## 💡 Next steps
Potential extensions:

- TLS support
- More message types & routing
- SO_REUSEPORT sharding