#include "swiftwire/client.hpp"
#include "swiftwire/protocol.hpp"

namespace swiftwire {
using namespace std::chrono_literals;
namespace proto = swiftwire::proto;

AsyncClient::AsyncClient(boost::asio::io_context& io)
    : io_(io), resolver_(io), socket_(io), timer_(io) {}

void AsyncClient::async_connect(const std::string& host,
                                const std::string& port,
                                std::chrono::milliseconds timeout,
                                ConnectHandler handler) {
    auto self = shared_from_this();
    auto done = std::make_shared<bool>(false);

    arm_timer(timeout, [this, self, done, handler] {
        if (*done) return;
        *done = true;
        boost::system::error_code ig;
        socket_.cancel(ig);
        socket_.close(ig);
        handler(make_error_code(boost::asio::error::timed_out));
    });

    resolver_.async_resolve(host, port,
        [this, self, done, handler](auto ec, auto results) {
            if (*done) return;
            if (ec) { *done = true; cancel_timer(); return handler(ec); }
            boost::asio::async_connect(socket_, results,
                [this, self, done, handler](auto ec2, auto) {
                    if (*done) return;
                    *done = true;
                    cancel_timer();
                    if (!ec2) {
                        boost::system::error_code ign;
                        socket_.set_option(tcp::no_delay(true), ign);
                    }
                    handler(ec2);
                });
        });
}

void AsyncClient::async_handshake(uint64_t client_id,
                                  std::chrono::milliseconds timeout,
                                  HelloHandler handler) {
    auto self = shared_from_this();
    auto done = std::make_shared<bool>(false);

    // Build request: [4B len=9][1B type][8B id]
    auto req = std::make_shared<std::array<char, 4 + 1 + 8>>();
    proto::write_u32be(req->data(), 1 + 8);
    (*req)[4] = static_cast<char>(proto::MSG_HELLO);
    proto::write_u64be(req->data() + 5, client_id);

    arm_timer(timeout, [this, self, done, handler] {
        if (*done) return;
        *done = true;
        boost::system::error_code ig;
        socket_.cancel(ig);
        handler(make_error_code(boost::asio::error::timed_out), 0, 0);
    });

    boost::asio::async_write(socket_, boost::asio::buffer(*req),
        [this, self, done, handler, client_id](auto ec, auto) {
            if (*done) return;
            if (ec) { *done = true; cancel_timer(); return handler(ec, 0, 0); }

            boost::asio::async_read(socket_, boost::asio::buffer(lenbuf_),
                [this, self, done, handler, client_id](auto ec2, auto) {
                    if (*done) return;
                    if (ec2) { *done = true; cancel_timer(); return handler(ec2, 0, 0); }

                    uint32_t blen = proto::read_u32be(lenbuf_.data());
                    constexpr uint32_t kMax = 1u << 20;
                    if (blen < (1 + 8 + 1) || blen > kMax) {
                        *done = true; cancel_timer();
                        return handler(make_error_code(boost::asio::error::invalid_argument), 0, 0);
                    }
                    body_.resize(blen);

                    boost::asio::async_read(socket_, boost::asio::buffer(body_),
                        [this, self, done, handler, client_id](auto ec3, auto) {
                            if (*done) return;
                            *done = true;
                            cancel_timer();
                            if (ec3) return handler(ec3, 0, 0);

                            if (body_.size() < 1 + 8 + 1)
                                return handler(make_error_code(boost::asio::error::invalid_argument), 0, 0);
                            uint8_t type = static_cast<uint8_t>(body_[0]);
                            if (type != proto::MSG_HELLO_ACK)
                                return handler(make_error_code(boost::asio::error::fault), 0, 0);

                            uint64_t echoed = proto::read_u64be(body_.data() + 1);
                            if (echoed != client_id)
                                return handler(make_error_code(boost::asio::error::fault), 0, 0);

                            uint8_t status = static_cast<uint8_t>(body_[1 + 8]);
                            return handler({}, echoed, status);
                        });
                });
        });
}

template <typename F>
void AsyncClient::arm_timer(std::chrono::milliseconds timeout, F on_timeout) {
    // boost::system::error_code ec;
    // timer_.expires_after(timeout, ec);
    timer_.expires_after(timeout);
    timer_.async_wait([on_timeout = std::move(on_timeout)](const boost::system::error_code& tec) mutable {
        if (tec == boost::asio::error::operation_aborted) return;
        on_timeout();
    });
}

void AsyncClient::cancel_timer() {
    // boost::system::error_code ig;
    // timer_.cancel(ig);
    timer_.cancel(); // modern Boost: no error_code overload
}

void AsyncClient::close() {
    cancel_timer();
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

// Explicit template instantiation for MSVC linkers (optional)
// template void AsyncClient::arm_timer<std::function<void()>>(std::chrono::milliseconds, std::function<void()>);

} // namespace swiftwire
