// Copyright (c) 2018, J.R. Versteegh <j.r.versteegh@orca-st.com>
// Copyright (c) 2017, Fizyr (https://fizyr.com)
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
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once
#ifndef MODBUS_CLIENT_H_
#define MODBUS_CLIENT_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include <boost/asio.hpp>

#include "functions.hpp"
#include "tcp.hpp"
#include "request.hpp"
#include "response.hpp"
#include "impl/serialize.hpp"
#include "impl/deserialize.hpp"

namespace modbus {

namespace asio = boost::asio;

struct Client  {
  typedef asio::ip::tcp tcp;

  template<typename T>
    using Callback = std::function<void (tcp_mbap const & header, T const & response, boost::system::error_code const &)>;

  /// Callback to invoke for IO errors that cants be linked to a specific transaction.
  /**
   * Additionally the connection will be closed and every transaction callback will be called with an EOF error.
   */
  std::function<void (boost::system::error_code const &)> on_io_error;

  Client(asio::io_service & ios): strand(ios), socket(ios), resolver(ios), ios_(ios), connected_(false) {}

  void connect(
      std::string const & hostname,
      int const & port,
      std::function<void(boost::system::error_code const &)> callback) {
    std::string ports = std::to_string(port);
    tcp::resolver::query query(hostname, ports);

    auto handler = strand.wrap(std::bind(&Client::on_resolve, this, std::placeholders::_1, std::placeholders::_2, callback));
    resolver.async_resolve(query, handler);
  }


  /// Connect to a server at the default Modbus port 502.
  void connect(
      std::string const & hostname,
      std::function<void(boost::system::error_code const &)> callback) {
    connect(hostname, 502, callback);
  }

  /// Disconnect from the server.
  /**
   * Any remaining transaction callbacks will be invoked with an EOF error.
   */
  void close() {
    // Call all remaining transaction handlers with operation_aborted, then clear transactions.
    for (auto & transaction : transactions) 
      transaction.second.handler(nullptr, 0, {}, asio::error::operation_aborted);
    transactions.clear();

    // Shutdown and close socket.
    boost::system::error_code error;
    resolver.cancel();
    socket.shutdown(tcp::socket::shutdown_both, error);
    connected_ = false;
    socket.close(error);
  }

  /// Reset the client.
  /**
   * Should be called before re-opening a connection after a previous connection was closed.
   */
  void reset() {
    // Clear buffers.
    read_buffer.consume(read_buffer.size());
    write_buffer.consume(write_buffer.size());

    // Old socket may hold now invalid file descriptor.
    socket = asio::ip::tcp::socket(ios_);
    connected_ = false;
  }


  /// Check if the connection to the server is open.
  /**
   * \return True if the connection to the server is open.
   */
  bool is_open() {
    return socket.is_open();
  }

  /// Check if the client is connected.
  bool is_connected() {
    return is_open() && connected_;
  }

  /// Read a number of coils from the connected server.
  void read_coils(
      std::uint8_t unit,
      std::uint16_t address,
      std::uint16_t count,
      Callback<response::read_coils> const & callback) {
    send_message(unit, request::read_coils{address, count}, callback);
  }

  /// Read a number of discrete inputs from the connected server.
  void read_discrete_inputs(
      std::uint8_t unit,
      std::uint16_t address,
      std::uint16_t count,
      Callback<response::read_discrete_inputs> const & callback) {
    send_message(unit, request::read_discrete_inputs{address, count}, callback);
  }


  /// Read a number of holding registers from the connected server.
  void read_holding_registers(
      std::uint8_t unit,
      std::uint16_t address,
      std::uint16_t count,
      Callback<response::read_holding_registers> const & callback) {
    send_message(unit, request::read_holding_registers{address, count}, callback);
  }


  /// Read a number of input registers from the connected server.
  void read_input_registers(
      std::uint8_t unit,
      std::uint16_t address,
      std::uint16_t count,
      Callback<response::read_input_registers> const & callback) {
    send_message(unit, request::read_input_registers{address, count}, callback);
  }


  /// Write to a single coil on the connected server.
  void write_single_coil(
      std::uint8_t unit,
      std::uint16_t address,
      bool value,          
      Callback<response::write_single_coil> const & callback) {
    send_message(unit, request::write_single_coil{address, value}, callback);
  }


  /// Write to a single register on the connected server.
  void write_single_register(
      std::uint8_t unit,
      std::uint16_t address,
      std::uint16_t value,
      Callback<response::write_single_register> const & callback) {
    send_message(unit, request::write_single_register{address, value}, callback);
  }


  /// Write to a number of coils on the connected server.
  void write_multiple_coils(
      std::uint8_t unit,
      std::uint16_t address,      
      std::vector<bool> values,      
      Callback<response::write_multiple_coils> const & callback) {
    send_message(unit, request::write_multiple_coils{address, values}, callback);
  }


  /// Write to a number of registers on the connected server.
  void write_multiple_registers(
      std::uint8_t unit,
      std::uint16_t address,
      std::vector<std::uint16_t> values,
      Callback<response::write_multiple_registers> const & callback) {
    send_message(unit, request::write_multiple_registers{address, values}, callback);
  }


  /// Perform a masked write to a register on the connected server.
  /**
   * Compliant servers will set the value of the register to:
   * ((old_value AND and_mask) OR (or_mask AND NOT and_MASK))
   */
  void mask_write_register(
      std::uint8_t unit,
      std::uint16_t address,
      std::uint16_t and_mask,
      std::uint16_t or_mask,
      Callback<response::mask_write_register> const & callback) {
    send_message(unit, request::mask_write_register{address, and_mask, or_mask}, callback);
  }


protected:
  /// Low level message handler.
  using Handler = std::function<std::uint8_t const * (
      std::uint8_t const * start, 
      std::size_t size, tcp_mbap 
      const & header, 
      boost::system::error_code error)>;

  // Make a handler that deserializes a messages and passes it to the user callback.
  template<typename T>
  Handler make_handler(Client::Callback<T>&& callback) {
    return [callback] (
        std::uint8_t const * start, 
        std::size_t length, 
        tcp_mbap const & header, 
        boost::system::error_code error) {
      T response;
      std::uint8_t const * current = start;
      std::uint8_t const * end = start + length;

      // Pass errors to callback.
      if (error) {
        callback(header, response, error);
        return current;
      }

      // Make sure the message contains atleast a function code.
      if (length < 1) {
        callback(header, response, modbus_error(errc::message_size_mismatch));
        return current;
      }

      // Function codes 128 and above are exception responses.
      if (*current >= 128) {
        callback(header, response, modbus_error(length >= 2 ? errc_t(start[1]) : errc::message_size_mismatch));
        return current;
      }

      // Try to deserialize the PDU.
      current = impl::deserialize(current, end - current, response, error);
      if (error) {
        callback(header, response, error);
        return current;
      }

      // Check response length consistency.
      // Length from the MBAP header includes the unit ID (1 byte) which is part of the MBAP header, not the response PDU.
      if (current - start != header.length - 1) {
        callback(header, response, modbus_error(errc::message_size_mismatch));
        return current;
      }

      callback(header, response, error);
      return current;
    };
  }

  /// Struct to hold transaction details.
  struct transaction_t {
    std::uint8_t function;
    Handler handler;
  };

  /// Strand to use to prevent concurrent handler execution.
  asio::io_service::strand strand;

  /// The socket to use.
  tcp::socket socket;

  /// The resolver to use.
  tcp::resolver resolver;

  /// Buffer for read operations.
  asio::streambuf read_buffer;

  /// Buffer for write operations.
  asio::streambuf write_buffer;

  /// Output iterator for write buffer.
  std::ostreambuf_iterator<char> output_iterator{&write_buffer};

  /// Transaction table to keep track of open transactions.
  std::map<int, transaction_t> transactions;

  /// Next transaction ID.
  std::uint16_t next_id = 0;

  /// Maintain reference to io service
  asio::io_service& ios_;

  /// Track connected state of client.
  bool connected_;


  /// Called when the resolver finished resolving a hostname.
  void on_resolve(
      boost::system::error_code const & error,
      tcp::resolver::iterator iterator,
      std::function<void(boost::system::error_code const &)> callback) {
    if (error) return 
      callback(error);

    auto handler = strand.wrap(std::bind(&Client::on_connect, this, std::placeholders::_1, std::placeholders::_2, callback));
    asio::async_connect(socket, iterator, handler);
  }


  /// Called when the socket finished connecting.
  void on_connect(
      boost::system::error_code const & error,
      tcp::resolver::iterator iterator,
      std::function<void(boost::system::error_code const &)> callback) {
    (void) iterator;

    if (callback) callback(error);

    // Start read loop if no error occured.
    if (!error) {
      connected_ = true;
      auto handler = strand.wrap(std::bind(&Client::on_read, this, std::placeholders::_1, std::placeholders::_2));
      socket.async_read_some(read_buffer.prepare(1024), handler);
    }
  }


  /// Called when the socket finished a read operation.
  void on_read(
      boost::system::error_code const & error,
      std::size_t bytes_transferred) {
    if (!connected_)
      return;
    if (error) {
      std::cout << "on_read" << std::endl;
      if (on_io_error) on_io_error(error);
      return;
    }

    read_buffer.commit(bytes_transferred);

    // Parse and process all complete messages in the buffer.
    while (process_message());

    // Read more data.
    auto handler = strand.wrap(std::bind(&Client::on_read, this, std::placeholders::_1, std::placeholders::_2));
    socket.async_read_some(read_buffer.prepare(1024), handler);
  }


  /// Called when the socket finished a write operation.
  void on_write(
      boost::system::error_code const & error,
      std::size_t bytes_transferred) {
    if (error) {
      if (on_io_error) 
        on_io_error(error);
      return;
    }

    write_buffer.consume(bytes_transferred);

    if (write_buffer.size()) {
      flush_write_buffer();
    }
  }


  /// Allocate a transaction in the transaction table.
  std::uint16_t allocate_transaction(std::uint8_t function, Handler handler) {
    std::uint16_t id = ++next_id;
    transactions.insert({id, {function, handler}});
    return id;
  }


  /// Parse and process a message from the read buffer.
  /**
   * \return True if a message was parsed succesfully, false if there was not enough data.
   */
  bool process_message() {
    /// Modbus/TCP MBAP header is 7 bytes.
    if (read_buffer.size() < 7) return false;

    uint8_t const * data = asio::buffer_cast<uint8_t const *>(read_buffer.data());

    boost::system::error_code error;
    tcp_mbap header;

    data = impl::deserialize(data, read_buffer.size(), header, error);

    // Handle deserialization errors in TCP MBAP.
    // Cant send an error to a specific transaction and can't continue to read from the connection.
    if (error) {
      if (on_io_error) on_io_error(error);
      close();
      return false;
    }

    // Ensure entire message is in buffer.
    if (read_buffer.size() < std::size_t(6 + header.length)) return false;

    auto transaction = transactions.find(header.transaction);
    if (transaction == transactions.end()) {
      // TODO: Transaction not found. Possibly call on_io_error?
      return true;
    }
    data = transaction->second.handler(data, header.length - 1, header, boost::system::error_code());
    transactions.erase(transaction);

    // Remove read data and handled transaction.
    read_buffer.consume(6 + header.length);

    return true;
  }

  /// Flush the write buffer.
  void flush_write_buffer() {
    auto handler = strand.wrap(std::bind(&Client::on_write, this, std::placeholders::_1, std::placeholders::_2));
    socket.async_write_some(write_buffer.data(), handler);
  }

  /// Send a Modbus request to the server.
  template<typename T>
    void send_message(
        std::uint8_t unit,                          ///< The unit identifier of the target device.
        T const & request,                          ///< The application data unit of the request.
        Callback<typename T::response> callback) {  ///< The callback to invoke when the reply arrives.
    strand.dispatch([this, unit, request, callback] () mutable {
      auto handler = make_handler<typename T::response>(std::move(callback));

      tcp_mbap header;
      header.transaction = allocate_transaction(request.function, handler);
      header.protocol    = 0;                    // 0 means Modbus.
      header.length      = request.length() + 1; // Unit ID is also counted in length field.
      header.unit        = unit;

      auto out = std::ostreambuf_iterator<char>(&write_buffer);
      impl::serialize(out, header);
      impl::serialize(out, request);
      flush_write_buffer();
    });
  }
};

}

#endif
// vim: autoindent syntax=cpp expandtab tabstop=2 softtabstop=2 shiftwidth=2
