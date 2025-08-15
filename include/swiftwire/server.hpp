#pragma once
#include <boost/asio.hpp>
#include <atomic>
#include <array>
#include <deque>
#include <memory>
#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>

namespace swiftwire {
using boost::asio::ip::tcp;

struct ServerConfig {
    std::size_t threads = std::max(1u, std::thread::hardware_concurrency());
    std::chrono::seconds idle_timeout{60};
    std::size_t max_frame = 1u << 20;              // 1 MiB
    std::size_t max_write_queue_bytes = 8u << 20;  // 8 MiB per connection
    bool tcp_nodelay = true;
};

class AsyncServer {
public:
    AsyncServer(boost::asio::io_context& io, const tcp::endpoint& ep, ServerConfig cfg = {});
    void run();   // start accepting
private:
    class Session;
    void do_accept();

private:
    boost::asio::io_context& io_;
    tcp::acceptor acceptor_;
    ServerConfig cfg_;
};

} // namespace swiftwire
