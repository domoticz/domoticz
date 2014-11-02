#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/signals2.hpp>

namespace tcp_proxy
{
	typedef boost::signals2::signal<void (const unsigned char *pData, const size_t Len)> OnProxyData;
	class bridge : public boost::enable_shared_from_this<bridge>
	{
	public:
		bridge(
			boost::asio::io_service& ios
			);
		boost::asio::ip::tcp::socket& downstream_socket();
		boost::asio::ip::tcp::socket& upstream_socket();

		void start(const std::string& upstream_host, const std::string& upstream_port);
		void handle_upstream_connect(const boost::system::error_code& error);

		OnProxyData sDownstreamData;
		OnProxyData sUpstreamData;
	private:
		void handle_downstream_write(const boost::system::error_code& error);
		void handle_downstream_read(const boost::system::error_code& error,const size_t& bytes_transferred);
		void handle_upstream_write(const boost::system::error_code& error);
		void handle_upstream_read(const boost::system::error_code& error,const size_t& bytes_transferred);
		void close();
		boost::asio::ip::tcp::socket downstream_socket_;
		boost::asio::ip::tcp::socket upstream_socket_;
		enum { max_data_length = 32*1024 }; //8KB
		unsigned char downstream_data_[max_data_length];
		unsigned char upstream_data_[max_data_length];
		boost::mutex mutex_;
	};
	class acceptor
	{
	public:
		typedef boost::shared_ptr<bridge> ptr_type;
		acceptor(
			const std::string& local_host, unsigned short local_port,
			const std::string& upstream_host, const std::string& upstream_port);
		~acceptor();
		bool accept_connections();
		bool start();
		bool stop();
		OnProxyData sOnDownstreamData;
		OnProxyData sOnUpstreamData;
	private:
		void handle_accept(const boost::system::error_code& error);
		void handle_stop();

		void OnUpstreamData(const unsigned char *pData, const size_t Len);
		void OnDownstreamData(const unsigned char *pData, const size_t Len);

		/// The io_service used to perform asynchronous operations.
		boost::asio::io_service io_service_;
		bool m_bDoStop;
		boost::asio::ip::address_v4 localhost_address;
		boost::asio::ip::tcp::acceptor acceptor_;
		ptr_type session_;
		std::string upstream_host_;
		std::string upstream_port_;
	};
}
