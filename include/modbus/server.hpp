// Copyright (c) 2018, J.R. Versteegh (https://www.orca-st.com)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the copyright holder(s) nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL J.R. Versteegh BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MODBUS_SERVER_H_
#define MODBUS_SERVER_H_
#pragma once

#include <string>
#include <array>
#include <cstdint>
#include <ostream>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#include <boost/asio/spawn.hpp>

#include "functions.hpp"
#include "tcp.hpp"
#include "request.hpp"
#include "response.hpp"

#include "../../src/impl/serialize.hpp"
#include "../../src/impl/deserialize.hpp"


namespace modbus {

namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;


template <typename ServerHandler>
struct server;

template <typename ServerHandler>
struct tcp_connection: public boost::enable_shared_from_this<tcp_connection<ServerHandler> > {
	typedef boost::shared_ptr<tcp_connection<ServerHandler> > pointer;

	static pointer create(asio::io_service& io_service, const boost::shared_ptr<ServerHandler>& handler) {
		return pointer(new tcp_connection<ServerHandler>(io_service, handler));
	}

	tcp::socket& socket() {
		return socket_;
	}

	void start(asio::yield_context yield) {
		while (socket_.is_open()) {
			read_data(yield);
		}
	}


private:
	tcp::socket socket_;
	boost::shared_ptr<ServerHandler> handler_;
	asio::io_service& io_service_;


	tcp_connection(asio::io_service& io_service, const boost::shared_ptr<ServerHandler>& handler): 
		socket_(io_service), handler_(handler), io_service_(io_service) {};


	void read_data(asio::yield_context yield) {
		asio::streambuf read_buffer;
		size_t bytes_transferred = socket_.async_read_some(read_buffer.prepare(1024), yield);
		read_buffer.commit(bytes_transferred);
		while (read_buffer.size() > 0) {
			handle_read(read_buffer, yield);
		}
	}


	void handle_read(asio::streambuf& read_buffer, asio::yield_context yield) {
		boost::system::error_code error;
		tcp_mbap header;

		const uint8_t* data = boost::asio::buffer_cast<const uint8_t*>(read_buffer.data());
		data = impl::deserialize(data, read_buffer.size(), header, error);
		if (error) {
			read_buffer.consume(read_buffer.size());
			return;
		}
		read_buffer.consume(header.size());
		if (header.length < 2) {
			write_error(header, errc::illegal_function, yield);
			return;
		}
		size_t data_size = static_cast<size_t>(header.length - 1);

		if (read_buffer.size() >= data_size) {
			try {
				handle_data(header, data, data_size, yield);
			}
			catch (modbus_exception& e) {
				write_error(header, e.get_error(), yield);
			}
		}
		else {
			write_error(header, errc::illegal_data_value, yield);
		}
		read_buffer.consume(data_size);
	}


	void handle_data(tcp_mbap header, const uint8_t* data, size_t data_size, asio::yield_context yield) {
		asio::streambuf write_buffer;
		auto out = std::ostreambuf_iterator<char>(&write_buffer);
		// Switch can probably be avoided by templatizing requests and responses or putting them in a type list
		switch (*data) {
			case functions::read_coils: 
				handle_request<request::read_coils>(out, header, data, data_size);
				break;
			case functions::read_discrete_inputs:;
				handle_request<request::read_discrete_inputs>(out, header, data, data_size);
				break;
			case functions::read_holding_registers:;
				handle_request<request::read_holding_registers>(out, header, data, data_size);
				break;
			case functions::read_input_registers:;
				handle_request<request::read_input_registers>(out, header, data, data_size);
				break;
			case functions::write_single_coil:;
				handle_request<request::write_single_coil>(out, header, data, data_size);
				break;
			case functions::write_single_register:;
				handle_request<request::write_single_register>(out, header, data, data_size);
				break;
			case functions::write_multiple_coils:;
				handle_request<request::write_multiple_coils>(out, header, data, data_size);
				break;
			case functions::write_multiple_registers:;
				handle_request<request::write_multiple_registers>(out, header, data, data_size);
				break;
			default:
				 throw modbus_exception(errc::illegal_function); 
		}
		asio::async_write(socket_, write_buffer, yield);
	}


	template <typename Request>
	void handle_request(std::ostreambuf_iterator<char>& out, tcp_mbap header,  const uint8_t* data, size_t data_size) {
		Request req;
		typename Request::response resp = handler_->handle(req);
		header.length = resp.length() + 1;
		impl::serialize(out, header);
		impl::serialize(out, resp);
	}


	void write_error(tcp_mbap header, errc::errc_t error, asio::yield_context yield) {
	}

};


struct default_handler {
	default_handler()
		: registers_(0x20000), coils_(0x20000) {}

	response::read_coils handle(const request::read_coils& req) {
		response::read_coils resp;
		resp.values.insert(
				resp.values.end(), 
				coils_.cbegin() + req.address,
				coils_.cbegin() + req.address + req.count
		);
		return resp;
	}

	response::read_discrete_inputs handle(const request::read_discrete_inputs& req) {
		response::read_discrete_inputs resp;
		for (int i = 0; i < req.count; ++i) {
			resp.values.push_back(false);
		}
		return resp;
	}

	response::read_holding_registers handle(const request::read_holding_registers& req) {
		response::read_holding_registers resp;
		resp.values.insert(
				resp.values.end(), 
				registers_.cbegin() + req.address,
				registers_.cbegin() + req.address + req.count
		);
		return resp;
	}

	response::read_input_registers handle(const request::read_input_registers& req) {
		response::read_input_registers resp;
		for (int i = 0; i < req.count; ++i) {
			resp.values.push_back(0);
		}
		return resp;
	}

	response::write_single_coil handle(const request::write_single_coil& req) {
		response::write_single_coil resp;
		coils_[req.address] = req.value;
		resp.address = req.address;
		resp.value = req.value;
	}

	response::write_single_register handle(const request::write_single_register& req) {
		response::write_single_register resp;
		registers_[req.address] = req.value;
		resp.address = req.address;
		resp.value = req.value;
	}

	response::write_multiple_coils handle(const request::write_multiple_coils& req) {
		response::write_multiple_coils resp;
		resp.address = req.address;
		resp.count = 0;
		auto iit = req.values.begin();
		auto oit = coils_.begin() + req.address;
		while (iit < req.values.end() && oit < coils_.end()) {
			*oit++ = *iit++;
			++resp.count;
		}
		return resp;
	}

	response::write_multiple_registers handle(const request::write_multiple_registers& req) {
		response::write_multiple_registers resp;
		resp.address = req.address;
		resp.count = 0;
		auto iit = req.values.begin();
		auto oit = registers_.begin() + req.address;
		while (iit < req.values.end() && oit < registers_.end()) {
			*oit++ = *iit++;
			++resp.count;
		}
		return resp;
	}
private:
	std::vector<std::uint16_t> registers_;
	std::vector<bool> coils_;
};

// A Modbus server base class
template <typename ServerHandler>
struct server: public boost::enable_shared_from_this<server<ServerHandler> > {
	server(asio::io_service& io_service, boost::shared_ptr<ServerHandler>& handler): 
		acceptor_(io_service, tcp::endpoint(tcp::v4(), 502)),
		handler_(handler) {
			start_accept();
		}

	protected:

	private:
	void start_accept() {
		auto  new_connection = tcp_connection<ServerHandler>::create(acceptor_.get_io_service(), handler_);

		acceptor_.async_accept(
				new_connection->socket(),
				boost::bind(&server<ServerHandler>::handle_accept, this, new_connection, asio::placeholders::error)
				);
	}

	void handle_accept(typename tcp_connection<ServerHandler>::pointer new_connection,
			const boost::system::error_code& error) {
		if (!error)
		{
			asio::spawn(acceptor_.get_io_service(), boost::bind(&tcp_connection<ServerHandler>::start, new_connection, _1));
		}

		start_accept();
	}

	tcp::acceptor acceptor_;
	boost::shared_ptr<ServerHandler> handler_;
};

}

#endif

// vim: autoindent syntax=cpp noexpandtab tabstop=4 softtabstop=4 shiftwidth=4
