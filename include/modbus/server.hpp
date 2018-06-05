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

#pragma once

#include <string>
#include <array>
#include <cstdint>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "functions.hpp"
#include "tcp.hpp"
#include "request.hpp"
#include "response.hpp"

#define BOOST_COROUTINES_NO_DEPRECATION_WARNING

namespace modbus {


using tcp = boost::asio::ip::tcp;

struct server;

struct tcp_connection: public boost::enable_shared_from_this<tcp_connection> {
  typedef boost::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_service& io_service, const boost::shared_ptr<server>& srv) {
    return pointer(new tcp_connection(io_service, srv));
  }

  tcp::socket& socket() {
    return socket_;
  }

  void start(boost::asio::yield_context yield) {
    try {
      boost::coroutines::coroutine<void>::pull_type handle_read{[this](boost::coroutines::coroutine<void>::push_type &sink) {
        handle_read_buffer();
        sink();
      }};
      while (socket_.is_open()) {
        size_t bytes_transferred = socket_.async_read_some(read_buffer_.prepare(1024), yield);
        read_buffer_.commit(bytes_transferred);
        handle_read();
      }
    }
    catch (std::exception& e) {
      // Send a modbus error response...
    }
  }

private:
  tcp::socket socket_;
  boost::shared_ptr<server> srv_;
  boost::asio::io_service& io_service_;
  boost::asio::streambuf read_buffer_;
  boost::asio::streambuf write_buffer_;

  tcp_connection(boost::asio::io_service& io_service, const boost::shared_ptr<server>& srv): 
      socket_(io_service), srv_(srv), io_service_(io_service) {};

  void handle_write(const boost::system::error_code& ec, size_t bytes_transferred) {
    if (!ec) {
      write_buffer_.consume(bytes_transferred);
    }
  }
  void handle_read_buffer() {
  }
};


// A Modbus server base class
struct server: public boost::enable_shared_from_this<server> {
  server(boost::asio::io_service& io_service): 
      acceptor_(io_service, tcp::endpoint(tcp::v4(), 502)),
      registers_(1024) {
    start_accept();
  }

protected:
  virtual response::write_multiple_registers handle_multiple_write_registers(request::write_multiple_registers& req) {
    response::write_multiple_registers resp;
    resp.address = req.address;
    resp.count = 0;
    auto iit = req.values.begin();
    auto oit = registers_.begin() + req.address;
    while (iit < req.values.end() && oit < registers_.end()) {
      *oit++ = *iit++;
      resp.count++;
    }
    return resp;
  }
  virtual response::read_holding_registers handle_read_holding_registers(request::read_holding_registers& req) {
    response::read_holding_registers resp;
    int count = 0;
    auto it = registers_.cbegin() + req.address;
    while (count < req.count && it < registers_.end()) {
      resp.values.push_back(*it++);
      count++;
    }
    return resp;
  }

private:
  void start_accept() {
    tcp_connection::pointer new_connection =
      tcp_connection::create(acceptor_.get_io_service(), shared_from_this());

    acceptor_.async_accept(
        new_connection->socket(),
        boost::bind(&server::handle_accept, this, new_connection, boost::asio::placeholders::error)
    );
  }

  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error) {
    if (!error)
    {
      boost::asio::spawn(acceptor_.get_io_service(), boost::bind(&tcp_connection::start, new_connection, _1));
    }

    start_accept();
  }

  tcp::acceptor acceptor_;
  std::vector<std::uint16_t> registers_;
};

}
