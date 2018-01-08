// Copyright Takatoshi Kondo 2017
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_TCP_ENDPOINT_HPP)
#define MQTT_TCP_ENDPOINT_HPP

#include <boost/asio.hpp>

#if !defined(MQTT_NO_TLS)
#include <boost/asio/ssl.hpp>
#endif // !defined(MQTT_NO_TLS)

#include <mqtt/utility.hpp>

namespace mqtt {

namespace as = boost::asio;

template <typename Socket, typename Strand>
class tcp_endpoint {
public:
    template <typename... Args>
    tcp_endpoint(as::io_service& ios, Args&&... args)
        :tcp_(ios, std::forward<Args>(args)...),
         strand_(ios) {
    }

    void close(boost::system::error_code& ec) {
        tcp_.lowest_layer().close(ec);
    }

    as::io_service& get_io_service() {
        return tcp_.get_io_service();
    }

    Socket& socket() { return tcp_; }
    Socket const& socket() const { return tcp_; }

    typename Socket::lowest_layer_type& lowest_layer() {
        return tcp_.lowest_layer();
    }

    template <typename T>
    void set_option(T&& t) {
        tcp_.set_option(std::forward<T>(t));
    }

    template <typename ConstBufferSequence, typename AcceptHandler>
    void async_accept(
        ConstBufferSequence const& buffers,
        AcceptHandler&& handler) {
        tcp_.async_accept(buffers, std::forward<AcceptHandler>(handler));
    }

#if !defined(MQTT_NO_TLS)
    template<typename HandshakeType, typename HandshakeHandler>
    void async_handshake(
        HandshakeType type,
        HandshakeHandler&& h) {
        tcp_.async_handshake(type, std::forward<HandshakeHandler>(h));
    }
#endif // !defined(MQTT_NO_TLS)

    template <typename MutableBufferSequence, typename ReadHandler>
    void async_read(
        MutableBufferSequence const& buffers,
        ReadHandler&& handler) {
        as::async_read(tcp_, buffers, strand_.wrap(std::forward<ReadHandler>(handler)));
    }

    template <typename ConstBufferSequence>
    std::size_t write(
        ConstBufferSequence const& buffers) {
        return as::write(tcp_, buffers);
    }

    template <typename ConstBufferSequence>
    std::size_t write(
        ConstBufferSequence const& buffers,
        boost::system::error_code& ec) {
        return as::write(tcp_, buffers, ec);
    }

    template <typename ConstBufferSequence, typename WriteHandler>
    void async_write(
        ConstBufferSequence const& buffers,
        WriteHandler&& handler) {
        as::async_write(
            tcp_,
            buffers,
            strand_.wrap(std::forward<WriteHandler>(handler))
        );
    }

    template <typename PostHandler>
    void post(PostHandler&& handler) {
        strand_.post(std::forward<PostHandler>(handler));
    }

private:
    Socket tcp_;
    Strand strand_;
};

template <typename Socket, typename Strand, typename MutableBufferSequence, typename ReadHandler>
inline void async_read(
    tcp_endpoint<Socket, Strand>& ep,
    MutableBufferSequence const& buffers,
    ReadHandler&& handler) {
    ep.async_read(buffers, std::forward<ReadHandler>(handler));
}

template <typename Socket, typename Strand, typename ConstBufferSequence>
inline std::size_t write(
    tcp_endpoint<Socket, Strand>& ep,
    ConstBufferSequence const& buffers) {
    return ep.write(buffers);
}

template <typename Socket, typename Strand, typename ConstBufferSequence>
inline std::size_t write(
    tcp_endpoint<Socket, Strand>& ep,
    ConstBufferSequence const& buffers,
    boost::system::error_code& ec) {
    return ep.write(buffers, ec);
}

template <typename Socket, typename Strand, typename ConstBufferSequence, typename WriteHandler>
inline void async_write(
    tcp_endpoint<Socket, Strand>& ep,
    ConstBufferSequence const& buffers,
    WriteHandler&& handler) {
    ep.async_write(buffers, std::forward<WriteHandler>(handler));
}

} // namespace mqtt

#endif // MQTT_TCP_ENDPOINT_HPP
