// Copyright Takatoshi Kondo 2017
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memory>
#include <boost/asio.hpp>

#if !defined(MQTT_SERVER_HPP)
#define MQTT_SERVER_HPP

#if !defined(MQTT_NO_TLS)
#include <boost/asio/ssl.hpp>
#endif // !defined(MQTT_NO_TLS)

#include <mqtt/tcp_endpoint.hpp>

#if defined(MQTT_USE_WS)
#include <mqtt/ws_endpoint.hpp>
#endif // defined(MQTT_USE_WS)

#include <mqtt/endpoint.hpp>
#include <mqtt/null_strand.hpp>

namespace mqtt {

namespace as = boost::asio;

template <typename Strand = as::io_service::strand, typename Mutex = std::mutex, template<typename...> class LockGuard = std::lock_guard>
class server {
public:
    using socket_t = tcp_endpoint<as::ip::tcp::socket, Strand>;
    using endpoint_t = endpoint<socket_t, Mutex, LockGuard>;
    using accept_handler = std::function<void(endpoint_t& ep)>;

    /**
     * @breif Error handler
     * @param ec error code
     */
    using error_handler = std::function<void(boost::system::error_code const& ec)>;

    template <typename AsioEndpoint, typename AcceptorConfig>
    server(
        AsioEndpoint&& ep,
        as::io_service& ios_accept,
        as::io_service& ios_con,
        AcceptorConfig&& config)
        : ios_accept_(ios_accept),
          ios_con_(ios_con),
          acceptor_(ios_accept_, std::forward<AsioEndpoint>(ep)),
          close_request_(false) {
        config(acceptor_);
    }

    template <typename AsioEndpoint>
    server(
        AsioEndpoint&& ep,
        as::io_service& ios_accept,
        as::io_service& ios_con)
        : server(std::forward<AsioEndpoint>(ep), ios_accept, ios_con, [](as::ip::tcp::acceptor&) {}) {}

    template <typename AsioEndpoint, typename AcceptorConfig>
    server(
        AsioEndpoint&& ep,
        as::io_service& ios,
        AcceptorConfig&& config)
        : server(std::forward<AsioEndpoint>(ep), ios, ios, std::forward<AcceptorConfig>(config)) {}

    template <typename AsioEndpoint>
    server(
        AsioEndpoint&& ep,
        as::io_service& ios)
        : server(std::forward<AsioEndpoint>(ep), ios, ios, [](as::ip::tcp::acceptor&) {}) {}

    void listen() {
        renew_socket();
        do_accept();
    }

    void close() {
        close_request_ = true;
        acceptor_.close();
    }

    void set_accept_handler(accept_handler h = accept_handler()) {
        h_accept_ = std::move(h);
    }

    /**
     * @brief Set error handler
     * @param h handler
     */
    void set_error_handler(error_handler h = error_handler()) {
        h_error_ = std::move(h);
    }

private:
    void renew_socket() {
        socket_.reset(new socket_t(ios_con_));
    }

    void do_accept() {
        if (close_request_) return;
        acceptor_.async_accept(
            socket_->lowest_layer(),
            [this]
            (boost::system::error_code const& ec) {
                if (ec) {
                    if (h_error_) h_error_(ec);
                    return;
                }
                auto sp = std::make_shared<endpoint_t>(std::move(socket_));
                if (h_accept_) h_accept_(*sp);
                renew_socket();
                do_accept();
            }
        );
    }

private:
    as::io_service& ios_accept_;
    as::io_service& ios_con_;
    as::ip::tcp::acceptor acceptor_;
    std::unique_ptr<socket_t> socket_;
    bool close_request_;
    accept_handler h_accept_;
    error_handler h_error_;
};

#if !defined(MQTT_NO_TLS)

template <typename Strand = as::io_service::strand, typename Mutex = std::mutex, template<typename...> class LockGuard = std::lock_guard>
class server_tls {
public:
    using socket_t = tcp_endpoint<as::ssl::stream<as::ip::tcp::socket>, Strand>;
    using endpoint_t = endpoint<socket_t, Mutex, LockGuard>;
    using accept_handler = std::function<void(endpoint_t& ep)>;

    /**
     * @breif Error handler
     * @param ec error code
     */
    using error_handler = std::function<void(boost::system::error_code const& ec)>;

    template <typename AsioEndpoint, typename AcceptorConfig>
    server_tls(
        AsioEndpoint&& ep,
        as::ssl::context&& ctx,
        as::io_service& ios_accept,
        as::io_service& ios_con,
        AcceptorConfig&& config)
        : ios_accept_(ios_accept),
          ios_con_(ios_con),
          acceptor_(ios_accept_, std::forward<AsioEndpoint>(ep)),
          close_request_(false),
          ctx_(std::move(ctx)) {
        config(acceptor_);
    }

    template <typename AsioEndpoint>
    server_tls(
        AsioEndpoint&& ep,
        as::ssl::context&& ctx,
        as::io_service& ios_accept,
        as::io_service& ios_con)
        : server_tls(std::forward<AsioEndpoint>(ep), std::move(ctx), ios_accept, ios_con, [](as::ip::tcp::acceptor&) {}) {}

    template <typename AsioEndpoint, typename AcceptorConfig>
    server_tls(
        AsioEndpoint&& ep,
        as::ssl::context&& ctx,
        as::io_service& ios,
        AcceptorConfig&& config)
        : server_tls(std::forward<AsioEndpoint>(ep), std::move(ctx), ios, ios, std::forward<AcceptorConfig>(config)) {}

    template <typename AsioEndpoint>
    server_tls(
        AsioEndpoint&& ep,
        as::ssl::context&& ctx,
        as::io_service& ios)
        : server_tls(std::forward<AsioEndpoint>(ep), std::move(ctx), ios, ios, [](as::ip::tcp::acceptor&) {}) {}

    void listen() {
        renew_socket();
        do_accept();
    }

    void close() {
        close_request_ = true;
        acceptor_.close();
    }

    void set_accept_handler(accept_handler h = accept_handler()) {
        h_accept_ = std::move(h);
    }

    /**
     * @brief Set error handler
     * @param h handler
     */
    void set_error_handler(error_handler h = error_handler()) {
        h_error_ = std::move(h);
    }

private:
    void renew_socket() {
        socket_.reset(new socket_t(ios_con_, ctx_));
    }

    void do_accept() {
        if (close_request_) return;
        acceptor_.async_accept(
            socket_->lowest_layer(),
            [this]
            (boost::system::error_code const& ec) {
                if (ec) {
                    if (h_error_) h_error_(ec);
                    return;
                }
                socket_->async_handshake(
                    as::ssl::stream_base::server,
                    [this]
                    (boost::system::error_code ec) {
                        if (ec) {
                            if (h_error_) h_error_(ec);
                            return;
                        }
                        auto sp = std::make_shared<endpoint_t>(std::move(socket_));
                        if (h_accept_) h_accept_(*sp);
                        renew_socket();
                        do_accept();
                    }
                );
            }
        );
    }

private:
    as::io_service& ios_accept_;
    as::io_service& ios_con_;
    as::ip::tcp::acceptor acceptor_;
    std::unique_ptr<socket_t> socket_;
    bool close_request_;
    accept_handler h_accept_;
    error_handler h_error_;
    as::ssl::context ctx_;
};

#endif // !defined(MQTT_NO_TLS)

#if defined(MQTT_USE_WS)

class set_subprotocols {
public:
    template <typename T>
    explicit
    set_subprotocols(T&& s)
        : s_(std::forward<T>(s)) {
    }
    template<bool isRequest, class Headers>
    void operator()(boost::beast::http::header<isRequest, Headers>& m) const {
        m.fields.replace("Sec-WebSocket-Protocol", s_);
    }
private:
    std::string s_;
};

template <typename Strand = as::io_service::strand, typename Mutex = std::mutex, template<typename...> class LockGuard = std::lock_guard>
class server_ws {
public:
    using socket_t = ws_endpoint<as::ip::tcp::socket, Strand>;
    using endpoint_t = endpoint<socket_t, Mutex, LockGuard>;
    using accept_handler = std::function<void(endpoint_t& ep)>;

    /**
     * @breif Error handler
     * @param ec error code
     */
    using error_handler = std::function<void(boost::system::error_code const& ec)>;

    template <typename AsioEndpoint, typename AcceptorConfig>
    server_ws(
        AsioEndpoint&& ep,
        as::io_service& ios_accept,
        as::io_service& ios_con,
        AcceptorConfig&& config)
        : ios_accept_(ios_accept),
          ios_con_(ios_con),
          acceptor_(ios_accept_, std::forward<AsioEndpoint>(ep)),
          close_request_(false) {
        config(acceptor_);
    }

    template <typename AsioEndpoint>
    server_ws(
        AsioEndpoint&& ep,
        as::io_service& ios_accept,
        as::io_service& ios_con)
        : server_ws(std::forward<AsioEndpoint>(ep), ios_accept, ios_con, [](as::ip::tcp::acceptor&) {}) {}

    template <typename AsioEndpoint, typename AcceptorConfig>
    server_ws(
        AsioEndpoint&& ep,
        as::io_service& ios,
        AcceptorConfig&& config)
        : server_ws(std::forward<AsioEndpoint>(ep), ios, ios, std::forward<AcceptorConfig>(config)) {}

    template <typename AsioEndpoint>
    server_ws(
        AsioEndpoint&& ep,
        as::io_service& ios)
        : server_ws(std::forward<AsioEndpoint>(ep), ios, ios, [](as::ip::tcp::acceptor&) {}) {}

    void listen() {
        renew_socket();
        do_accept();
    }

    void close() {
        close_request_ = true;
        acceptor_.close();
    }

    void set_accept_handler(accept_handler h = accept_handler()) {
        h_accept_ = std::move(h);
    }

    /**
     * @brief Set error handler
     * @param h handler
     */
    void set_error_handler(error_handler h = error_handler()) {
        h_error_ = std::move(h);
    }

private:
    void renew_socket() {
        socket_.reset(new socket_t(ios_con_));
    }

    void do_accept() {
        if (close_request_) return;
        acceptor_.async_accept(
            socket_->next_layer(),
            [this]
            (boost::system::error_code const& ec) {
                if (ec) {
                    if (h_error_) h_error_(ec);
                    return;
                }
                auto sb = std::make_shared<boost::asio::streambuf>();
                auto request = std::make_shared<boost::beast::http::request<boost::beast::http::string_body>>();
                boost::beast::http::async_read(
                    socket_->next_layer(),
                    *sb,
                    *request,
                    [this, sb, request]
                    (boost::system::error_code const& ec, std::size_t) {
                        if (ec) {
                            if (h_error_) h_error_(ec);
                            return;
                        }
                        if (!boost::beast::websocket::is_upgrade(*request)) {
                            if (h_error_) h_error_(boost::system::errc::make_error_code(boost::system::errc::protocol_error));
                            return;
                        }
                        socket_->async_accept_ex(
                            *request,
                            [request]
                            (boost::beast::websocket::response_type& m) {
                                auto it = request->find("Sec-WebSocket-Protocol");
                                if (it != request->end()) {
                                    m.insert(it->name(), it->value());
                                }
                            },
                            [this]
                            (boost::system::error_code const& ec) {
                                if (ec) {
                                    if (h_error_) h_error_(ec);
                                    return;
                                }
                                auto sp = std::make_shared<endpoint_t>(std::move(socket_));
                                if (h_accept_) h_accept_(*sp);
                                renew_socket();
                                do_accept();
                            }
                        );
                    }
                );
            }
        );
    }

private:
    as::io_service& ios_accept_;
    as::io_service& ios_con_;
    as::ip::tcp::acceptor acceptor_;
    std::unique_ptr<socket_t> socket_;
    bool close_request_;
    accept_handler h_accept_;
    error_handler h_error_;
};


#if !defined(MQTT_NO_TLS)

template <typename Strand = as::io_service::strand, typename Mutex = std::mutex, template<typename...> class LockGuard = std::lock_guard>
class server_tls_ws {
public:
    using socket_t = mqtt::ws_endpoint<as::ssl::stream<as::ip::tcp::socket>, Strand>;
    using endpoint_t = endpoint<socket_t, Mutex, LockGuard>;

    using accept_handler = std::function<void(endpoint_t& ep)>;

    /**
     * @breif Error handler
     * @param ec error code
     */
    using error_handler = std::function<void(boost::system::error_code const& ec)>;

    template <typename AsioEndpoint, typename AcceptorConfig>
    server_tls_ws(
        AsioEndpoint&& ep,
        as::ssl::context&& ctx,
        as::io_service& ios_accept,
        as::io_service& ios_con,
        AcceptorConfig&& config)
        : ios_accept_(ios_accept),
          ios_con_(ios_con),
          acceptor_(ios_accept_, std::forward<AsioEndpoint>(ep)),
          close_request_(false),
          ctx_(std::move(ctx)) {
        config(acceptor_);
    }

    template <typename AsioEndpoint>
    server_tls_ws(
        AsioEndpoint&& ep,
        as::ssl::context&& ctx,
        as::io_service& ios_accept,
        as::io_service& ios_con)
        : server_tls_ws(std::forward<AsioEndpoint>(ep), std::move(ctx), ios_accept, ios_con, [](as::ip::tcp::acceptor&) {}) {}

    template <typename AsioEndpoint, typename AcceptorConfig>
    server_tls_ws(
        AsioEndpoint&& ep,
        as::ssl::context&& ctx,
        as::io_service& ios,
        AcceptorConfig&& config)
        : server_tls_ws(std::forward<AsioEndpoint>(ep), std::move(ctx), ios, ios, std::forward<AcceptorConfig>(config)) {}

    template <typename AsioEndpoint>
    server_tls_ws(
        AsioEndpoint&& ep,
        as::ssl::context&& ctx,
        as::io_service& ios)
        : server_tls_ws(std::forward<AsioEndpoint>(ep), std::move(ctx), ios, ios, [](as::ip::tcp::acceptor&) {}) {}

    void listen() {
        renew_socket();
        do_accept();
    }

    void close() {
        close_request_ = true;
        acceptor_.close();
    }

    void set_accept_handler(accept_handler h = accept_handler()) {
        h_accept_ = std::move(h);
    }

    /**
     * @brief Set error handler
     * @param h handler
     */
    void set_error_handler(error_handler h = error_handler()) {
        h_error_ = std::move(h);
    }

private:
    void renew_socket() {
        socket_.reset(new socket_t(ios_con_, ctx_));
    }

    void do_accept() {
        if (close_request_) return;
        acceptor_.async_accept(
            socket_->next_layer().next_layer(),
            [this]
            (boost::system::error_code const& ec) {
                if (ec) {
                    if (h_error_) h_error_(ec);
                    return;
                }
                socket_->next_layer().async_handshake(
                    as::ssl::stream_base::server,
                    [this]
                    (boost::system::error_code ec) {
                        if (ec) {
                            if (h_error_) h_error_(ec);
                            return;
                        }
                        auto sb = std::make_shared<boost::asio::streambuf>();
                        auto request = std::make_shared<boost::beast::http::request<boost::beast::http::string_body>>();
                        boost::beast::http::async_read(
                            socket_->next_layer(),
                            *sb,
                            *request,
                            [this, sb, request]
                            (boost::system::error_code const& ec, std::size_t) {
                                if (ec) {
                                    if (h_error_) h_error_(ec);
                                    return;
                                }
                                if (!boost::beast::websocket::is_upgrade(*request)) {
                                    if (h_error_) h_error_(boost::system::errc::make_error_code(boost::system::errc::protocol_error));
                                    return;
                                }
                                socket_->async_accept_ex(
                                    *request,
                                    [request]
                                    (boost::beast::websocket::response_type& m) {
                                        auto it = request->find("Sec-WebSocket-Protocol");
                                        if (it != request->end()) {
                                            m.insert(it->name(), it->value());
                                        }
                                    },
                                    [this]
                                    (boost::system::error_code const& ec) {
                                        if (ec) {
                                            if (h_error_) h_error_(ec);
                                            return;
                                        }
                                        auto sp = std::make_shared<endpoint_t>(std::move(socket_));
                                        if (h_accept_) h_accept_(*sp);
                                        renew_socket();
                                        do_accept();
                                    }
                                );
                            }
                        );
                    }
                );
            }
        );
    }

private:
    as::io_service& ios_accept_;
    as::io_service& ios_con_;
    as::ip::tcp::acceptor acceptor_;
    std::unique_ptr<socket_t> socket_;
    bool close_request_;
    accept_handler h_accept_;
    error_handler h_error_;
    as::ssl::context ctx_;
};

#endif // !defined(MQTT_NO_TLS)

#endif // defined(MQTT_USE_WS)

} // namespace mqtt

#endif // MQTT_SERVER_HPP
