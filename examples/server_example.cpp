#include "swiftwire/server.hpp"
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    const std::string host = (argc > 1) ? argv[1] : "0.0.0.0";
    const std::string port = (argc > 2) ? argv[2] : "9000";

    boost::asio::io_context io;
    swiftwire::ServerConfig cfg;
    cfg.threads = std::max(1u, std::thread::hardware_concurrency());

    try {
        swiftwire::AsyncServer server(
            io,
            { boost::asio::ip::make_address(host), static_cast<unsigned short>(std::stoi(port)) },
            cfg
        );
        server.run();

        // Run io_context on N threads
        std::vector<std::thread> workers;
        for (std::size_t i = 1; i < cfg.threads; ++i) {
            workers.emplace_back([&]{ io.run(); });
        }

        // Graceful shutdown
        boost::asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io.stop(); });

        io.run();
        for (auto& t : workers) t.join();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
