#include "swiftwire/server.hpp"
#include "swiftwire/protocol.hpp"
#include <boost/asio/signal_set.hpp>

namespace swiftwire {
namespace proto = swiftwire::proto;

class AsyncServer::Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, const ServerConfig& cfg)
        : socket_(std::move(socket)), timer_(socket_.get_executor()), cfg_(cfg) {}

    void start() {
        boost::system::error_code ec;
        if (cfg_.tcp_nodelay) socket_.set_option(tcp::no_delay(true), ec);
        refresh_timer();
        read_len();
    }

private:
    void refresh_timer() {
        timer_.expires_after(cfg_.idle_timeout);
        auto self = shared_from_this();
        timer_.async_wait([self](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted) return;
            self->fail_and_close(boost::asio::error::timed_out);
        });
    }
    void cancel_timer() { boost::system::error_code ig; timer_.cancel(ig); }

    void read_len() {
        auto self = shared_from_this();
        refresh_timer();
        boost::asio::async_read(socket_, boost::asio::buffer(lenbuf_),
            [self](auto ec, std::size_t) {
                if (ec) return self->fail_and_close(ec);
                uint32_t blen = proto::read_u32be(self->lenbuf_.data());
                if (blen == 0 || blen > self->cfg_.max_frame)
                    return self->fail_and_close(boost::asio::error::message_size);
                self->body_.resize(blen);
                self->read_body();
            });
    }
    void read_body() {
        auto self = shared_from_this();
        refresh_timer();
        boost::asio::async_read(socket_, boost::asio::buffer(body_),
            [self](auto ec, std::size_t) {
                if (ec) return self->fail_and_close(ec);
                self->handle_message();
                self->read_len();
            });
    }

    void handle_message() {
        cancel_timer();
        if (body_.empty()) return;

        uint8_t type = static_cast<uint8_t>(body_[0]);
        switch (type) {
            case proto::MSG_HELLO: {
                if (body_.size() < 1 + 8) return; // ignore malformed
                uint64_t client_id = proto::read_u64be(body_.data() + 1);
                send_hello_ack(client_id, /*status=*/0);
                break;
            }
            default: {
                if (body_.size() >= 1 + 8) {
                    uint64_t maybe_id = proto::read_u64be(body_.data() + 1);
                    send_hello_ack(maybe_id, /*status=*/1);
                }
                break;
            }
        }
    }

    void send_hello_ack(uint64_t id, uint8_t status) {
        constexpr std::size_t body_len = 1 + 8 + 1;
        auto buf = std::make_shared<std::vector<char>>(4 + body_len);
        proto::write_u32be(buf->data(), static_cast<uint32_t>(body_len));
        (*buf)[4] = static_cast<char>(proto::MSG_HELLO_ACK);
        proto::write_u64be(buf->data() + 5, id);
        (*buf)[13] = static_cast<char>(status);
        enqueue_write(std::move(buf));
    }

    void enqueue_write(std::shared_ptr<std::vector<char>> buf) {
        pending_bytes_ += buf->size();
        if (pending_bytes_ > cfg_.max_write_queue_bytes)
            return fail_and_close(boost::asio::error::no_buffer_space);
        bool idle = write_queue_.empty();
        write_queue_.push_back(std::move(buf));
        if (idle) do_write();
    }

    void do_write() {
        if (write_queue_.empty()) return;
        auto self = shared_from_this();
        refresh_timer();
        auto& front = write_queue_.front();
        boost::asio::async_write(socket_, boost::asio::buffer(*front),
            [self](auto ec, std::size_t n) {
                self->pending_bytes_ -= std::min<std::size_t>(n, self->write_queue_.front()->size());
                self->write_queue_.pop_front();
                if (ec) return self->fail_and_close(ec);
                if (!self->write_queue_.empty()) self->do_write();
            });
    }

    void fail_and_close(const boost::system::error_code& ec) {
        if (closed_.exchange(true)) return;
        cancel_timer();
        boost::system::error_code ig;
        socket_.shutdown(tcp::socket::shutdown_both, ig);
        socket_.close(ig);
        (void)ec; // optionally log
    }

private:
    tcp::socket socket_;
    boost::asio::steady_timer timer_;
    const ServerConfig cfg_;

    std::array<char, 4> lenbuf_{};
    std::vector<char> body_;
    std::deque<std::shared_ptr<std::vector<char>>> write_queue_;
    std::size_t pending_bytes_{0};
    std::atomic<bool> closed_{false};
};

AsyncServer::AsyncServer(boost::asio::io_context& io, const tcp::endpoint& ep, ServerConfig cfg)
    : io_(io), acceptor_(io), cfg_(cfg) {
    boost::system::error_code ec;
    acceptor_.open(ep.protocol(), ec);
    if (ec) throw boost::system::system_error(ec);
    acceptor_.set_option(tcp::acceptor::reuse_address(true), ec);
    if (ec) throw boost::system::system_error(ec);
    acceptor_.bind(ep, ec);
    if (ec) throw boost::system::system_error(ec);
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) throw boost::system::system_error(ec);
}

void AsyncServer::run() {
    do_accept();
}

void AsyncServer::do_accept() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& ec, tcp::socket socket) {
            if (!ec) std::make_shared<Session>(std::move(socket), cfg_)->start();
            do_accept();
        });
}

} // namespace swiftwire
