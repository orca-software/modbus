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
#ifndef MODBUS_ERROR_H_
#define MODBUS_ERROR_H_

#include <boost/system/error_code.hpp>

#include <exception>

namespace modbus {

/// Modbus error code constants.
namespace errc {
  enum errc_t {
    illegal_function                        = 0x01,
    illegal_data_address                    = 0x02,
    illegal_data_value                      = 0x03,
    server_device_failure                   = 0x04,
    acknowledge                             = 0x05,
    server_device_busy                      = 0x06,
    memory_parity_error                     = 0x08,
    gateway_path_unavailable                = 0x0a,
    gateway_target_device_failed_to_respond = 0x0b,

    message_size_mismatch                   = 0x1001,
    message_too_large                       = 0x1002,
    unexpected_function_code                = 0x1003,
    invalid_value                           = 0x1004,
  };
}

/// Error category for modbus errors.
class Modbus_category : public boost::system::error_category {
  /// Get the name of the error category
  char const * name() const noexcept override {
    return "modbus";
  }

  /// Get a descriptive error message for an error code.
  std::string message(int error) const noexcept override {
    switch (errc::errc_t(error)) {
      case errc::illegal_function:                        return "error 01: Illegal function";
      case errc::illegal_data_address:                    return "error 02: Illegal data address";
      case errc::illegal_data_value:                      return "error 03: Illegal data value";
      case errc::server_device_failure:                   return "error 04: Server device failure";
      case errc::acknowledge:                             return "error 05: Acknowledge";
      case errc::server_device_busy:                      return "error 06: Server device busy";
      case errc::memory_parity_error:                     return "error 08: Memory parity error";
      case errc::gateway_path_unavailable:                return "error 10: Gateway path unavailable";
      case errc::gateway_target_device_failed_to_respond: return "error 11: Gateway target device failed to respond";

      case errc::message_size_mismatch:                   return "peer error: message size mismatch";
      case errc::message_too_large:                       return "peer error: message size limit exceeded";
      case errc::unexpected_function_code:                return "peer error: unexpected function code";
      case errc::invalid_value:                           return "peer error: invalid value received";
    }

    return "unknown error: " + std::to_string(error);
  }
};

/// Enum type for Modbus error codes.
using errc_t = errc::errc_t;

/// The error category for modbus errors.
static boost::system::error_category const & modbus_category() {
  static Modbus_category modbus_category;
  return modbus_category;
}

/// Get an error code for a Modbus error,
inline boost::system::error_code modbus_error(modbus::errc_t error_code) {
  return boost::system::error_code(error_code, modbus_category());
}

struct modbus_exception: public std::runtime_error {
  modbus_exception(errc_t error): runtime_error("modbus error"), error_(error) {}
  modbus_exception(int error): runtime_error("modbus error"), error_(static_cast<errc_t>(error)) {}
  errc_t get_error() {
    return error_;
  }
private:
  errc_t error_;
};

}  // namespace modbus

#endif
// vim: autoindent syntax=cpp expandtab tabstop=2 softtabstop=2 shiftwidth=2
