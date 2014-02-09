//
// tcpproxy_server_v03.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2007 Arash Partow (http://www.partow.net)
// URL: http://www.partow.net/programming/tcpproxy/index.html
//
// Distributed under the Boost Software License, Version 1.0.
//
//
// Modified for Domoticz
//
//

#include "stdafx.h"
#include "tcpproxy_server.h"

namespace tcp_proxy
{
	bridge::bridge(boost::asio::io_service& ios)
      : downstream_socket_(ios),
        upstream_socket_(ios)
	{
	}

	boost::asio::ip::tcp::socket& bridge::downstream_socket()
	{
		return downstream_socket_;
	}

	boost::asio::ip::tcp::socket& bridge::upstream_socket()
	{
		return upstream_socket_;
	}

	void bridge::start(const std::string& upstream_host, const std::string& upstream_port)
	{
		boost::asio::ip::tcp::endpoint end;


		boost::asio::io_service &ios=downstream_socket_.get_io_service();
		boost::asio::ip::tcp::resolver resolver(ios);
		boost::asio::ip::tcp::resolver::query query(upstream_host, upstream_port, boost::asio::ip::resolver_query_base::numeric_service);
		boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
		if (i == boost::asio::ip::tcp::resolver::iterator())
		{
			end=boost::asio::ip::tcp::endpoint(
				boost::asio::ip::address::from_string(upstream_host),
				(unsigned short)atoi(upstream_port.c_str()));
		}
		else
			end=*i;
		upstream_socket_.async_connect(
			end,
			boost::bind(&bridge::handle_upstream_connect,
				shared_from_this(),
				boost::asio::placeholders::error));
	}

	void bridge::handle_upstream_connect(const boost::system::error_code& error)
	{
		if (!error)
		{
		upstream_socket_.async_read_some(
				boost::asio::buffer(upstream_data_,max_data_length),
				boost::bind(&bridge::handle_upstream_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

		downstream_socket_.async_read_some(
				boost::asio::buffer(downstream_data_,max_data_length),
				boost::bind(&bridge::handle_downstream_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		close();
	}

	void bridge::handle_downstream_write(const boost::system::error_code& error)
	{
		if (!error)
		{
		upstream_socket_.async_read_some(
				boost::asio::buffer(upstream_data_,max_data_length),
				boost::bind(&bridge::handle_upstream_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		close();
	}

	void bridge::handle_downstream_read(const boost::system::error_code& error,const size_t& bytes_transferred)
	{
		if (!error)
		{
			//boost::mutex::scoped_lock lock(mutex_);
			sDownstreamData(reinterpret_cast<unsigned char*>(&downstream_data_[0]),static_cast<size_t>(bytes_transferred));
			async_write(upstream_socket_,
					boost::asio::buffer(downstream_data_,bytes_transferred),
					boost::bind(&bridge::handle_upstream_write,
						shared_from_this(),
						boost::asio::placeholders::error));
		}
		else
			close();
	}

	void bridge::handle_upstream_write(const boost::system::error_code& error)
	{
		if (!error)
		{
			downstream_socket_.async_read_some(
					boost::asio::buffer(downstream_data_,max_data_length),
					boost::bind(&bridge::handle_downstream_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
		}
		else
			close();
	}

	void bridge::handle_upstream_read(const boost::system::error_code& error,
							const size_t& bytes_transferred)
	{
		if (!error)
		{
			//boost::mutex::scoped_lock lock(mutex_);
			sUpstreamData(reinterpret_cast<unsigned char*>(&upstream_data_[0]),static_cast<size_t>(bytes_transferred));

			async_write(downstream_socket_,
					boost::asio::buffer(upstream_data_,bytes_transferred),
					boost::bind(&bridge::handle_downstream_write,
						shared_from_this(),
						boost::asio::placeholders::error));
		}
		else
			close();
	}

	void bridge::close()
	{
		boost::mutex::scoped_lock lock(mutex_);
		if (downstream_socket_.is_open())
		{
			downstream_socket_.close();
		}
		if (upstream_socket_.is_open())
		{
			upstream_socket_.close();
		}
	}
//Acceptor Class
	acceptor::acceptor(
			const std::string& local_host, unsigned short local_port,
			const std::string& upstream_host, const std::string& upstream_port)
	:	io_service_(),
		localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
		acceptor_(io_service_,boost::asio::ip::tcp::endpoint(localhost_address,local_port)),
		upstream_port_(upstream_port),
		upstream_host_(upstream_host)
	{

	}
	acceptor::~acceptor()
	{

	}

	bool acceptor::accept_connections()
	{
		try
		{
			session_ = boost::shared_ptr<bridge>(
				new bridge(io_service_)
			);
			session_->sDownstreamData.connect( boost::bind( &acceptor::OnDownstreamData, this, _1, _2 ) );
			session_->sUpstreamData.connect( boost::bind( &acceptor::OnUpstreamData, this, _1, _2 ) );

			acceptor_.async_accept(session_->downstream_socket(),
				boost::bind(&acceptor::handle_accept,
						this,
						boost::asio::placeholders::error));
		}
		catch(std::exception& e)
		{
			std::cerr << "acceptor exception: " << e.what() << std::endl;
			return false;
		}
		return true;
	}
	bool acceptor::start()
	{
		m_bDoStop=false;

		accept_connections();
		// The io_service::run() call will block until all asynchronous operations
		// have finished. While the server is running, there is always at least one
		// asynchronous operation outstanding: the asynchronous accept call waiting
		// for new incoming connections.
		io_service_.run();
		return true;
	}
	bool acceptor::stop()
	{
		m_bDoStop=true;
		// Post a call to the stop function so that server::stop() is safe to call
		// from any thread.
		io_service_.post(boost::bind(&acceptor::handle_stop, this));
		return true;
	}

	void acceptor::handle_stop()
	{
		// The server is stopped by canceling all outstanding asynchronous
		// operations. Once all operations have finished the io_service::run() call
		// will exit.
		acceptor_.close();
		//connection_manager_.stop_all();
	}

	void acceptor::handle_accept(const boost::system::error_code& error)
	{
		if (!error)
		{
			session_->start(upstream_host_,upstream_port_);
			if (!accept_connections())
			{
				std::cerr << "Failure during call to accept." << std::endl;
			}
		}
		else
		{
			if (!m_bDoStop)
				std::cerr << "Error: " << error.message() << std::endl;
		}
	}

	void acceptor::OnUpstreamData(const unsigned char *pData, const size_t Len)
	{
		sOnDownstreamData(pData,Len);
	}
	void acceptor::OnDownstreamData(const unsigned char *pData, const size_t Len)
	{
		sOnUpstreamData(pData,Len);
	}

} //end namespace
