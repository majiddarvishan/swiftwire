#include "swiftwire/client.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    const std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    const std::string port = (argc > 2) ? argv[2] : "9000";
    uint64_t client_id = (argc > 3) ? std::strtoull(argv[3], nullptr, 10) : 42;

    boost::asio::io_context io;
    auto client = std::make_shared<swiftwire::AsyncClient>(io);

    client->async_connect(host, port, std::chrono::seconds(5),
        [client, client_id](auto ec) {
            if (ec) { std::cerr << "Connect failed: " << ec.message() << "\n"; return; }
            std::cout << "Connected.\n";
            client->async_handshake(client_id, std::chrono::seconds(5),
                [](auto ec2, uint64_t id, uint8_t status) {
                    if (ec2) std::cerr << "Handshake failed: " << ec2.message() << "\n";
                    else std::cout << "HELLO_ACK: id=" << id << " status=" << int(status) << "\n";
                });
        });

    io.run();
    return 0;
}
