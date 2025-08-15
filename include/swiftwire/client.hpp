#pragma once
#include <boost/asio.hpp>
#include <array>
#include <vector>
#include <functional>
#include <cstdint>
#include <chrono>

namespace swiftwire {
using boost::asio::ip::tcp;

class AsyncClient : public std::enable_shared_from_this<AsyncClient> {
public:
    using ConnectHandler  = std::function<void(const boost::system::error_code&)>;
    using HelloHandler    = std::function<void(const boost::system::error_code&, uint64_t /*id*/, uint8_t /*status*/)>;

    explicit AsyncClient(boost::asio::io_context& io);

    // Resolve + connect with deadline (single-thread-friendly)
    void async_connect(const std::string& host,
                       const std::string& port,
                       std::chrono::milliseconds timeout,
                       ConnectHandler handler);

    // Send HELLO and await HELLO_ACK with deadline
    void async_handshake(uint64_t client_id,
                         std::chrono::milliseconds timeout,
                         HelloHandler handler);

    // Graceful close
    void close();

private:
    template <typename F>
    void arm_timer(std::chrono::milliseconds timeout, F on_timeout);
    void cancel_timer();

private:
    boost::asio::io_context& io_;
    tcp::resolver resolver_;
    tcp::socket   socket_;
    boost::asio::steady_timer timer_;
    std::array<char, 4> lenbuf_{};
    std::vector<char>   body_;
};

using AsyncClientPtr = std::shared_ptr<AsyncClient>;

} // namespace swiftwire
