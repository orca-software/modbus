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
#ifndef MODBUS_RESPONSE_H_
#define MODBUS_RESPONSE_H_

#include <cstdint>
#include <vector>

#include "functions.hpp"

namespace modbus {

namespace request {
	struct read_coils;
	struct read_discrete_inputs;
	struct read_holding_registers;
	struct read_input_registers;
	struct write_single_coil;
	struct write_single_register;
	struct write_multiple_coils;
	struct write_multiple_registers;
	struct mask_write_register;
}

namespace response {

	/// Message representing a read_coils response.
	struct read_coils {
		/// Request type.
		using request = request::read_coils;

		/// The function code.
		static constexpr std::uint8_t function = functions::read_coils;

		/// The read values.
		std::vector<bool> values;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 2 + (values.size() + 7) / 8;
		}
	};

	/// Message representing a read_discrete_inputs response.
	struct read_discrete_inputs {
		/// Request type.
		using request = request::read_discrete_inputs;

		/// The function code.
		static constexpr std::uint8_t function = functions::read_discrete_inputs;

		/// The read values.
		std::vector<bool> values;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 2 + (values.size() + 7) / 8;
		}
	};

	/// Message representing a read_holding_registers response.
	struct read_holding_registers {
		/// Request type.
		using request = request::read_holding_registers;

		/// The function code.
		static constexpr std::uint8_t function = functions::read_holding_registers;

		/// The read values.
		std::vector<std::uint16_t> values;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 2 + values.size() * 2;
		}
	};

	/// Message representing a read_input_registers response.
	struct read_input_registers {
		/// Request type.
		using request = request::read_input_registers;

		/// The function code.
		static constexpr std::uint8_t function = functions::read_input_registers;

		/// The read values.
		std::vector<std::uint16_t> values;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 2 + values.size() * 2;
		}
	};

	/// Message representing a write_single_coil response.
	struct write_single_coil {
		/// Request type.
		using request = request::write_single_coil;

		/// The function code.
		static constexpr std::uint8_t function = functions::write_single_coil;

		/// The address of the coil written to.
		std::uint16_t address;

		/// The value written to the coil.
		bool value;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a write_single_register response.
	struct write_single_register {
		/// Request type.
		using request = request::write_single_register;

		/// The function code.
		static constexpr std::uint8_t function = functions::write_single_register;

		/// The address of the register written to.
		std::uint16_t address;

		/// The value written to the register.
		std::uint16_t value;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a write_multiple_coil response.
	struct write_multiple_coils {
		/// Request type.
		using request = request::write_multiple_coils;

		/// The function code.
		static constexpr std::uint8_t function = functions::write_multiple_coils;

		/// The address of the first coil written to.
		std::uint16_t address;

		/// The number of coils written to.
		std::uint16_t count;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a write_multiple_registers response.
	struct write_multiple_registers {
		/// Request type.
		using request = request::write_multiple_registers;

		/// The function code.
		static constexpr std::uint8_t function = functions::write_multiple_registers;

		/// The address of the first register written to.
		std::uint16_t address;

		/// The number of registers written to.
		std::uint16_t count;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a mask_write_register response.
	struct mask_write_register {
		/// Request type.
		using request = request::mask_write_register;

		/// The function code.
		static constexpr std::uint8_t function = functions::mask_write_register;

		/// The address of the register written to.
		std::uint16_t address;

		/// The AND mask used.
		std::uint16_t and_mask;

		/// The OR mask used.
		std::uint16_t or_mask;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 7;
		}
	};

}}

#endif
// vim: autoindent syntax=cpp expandtab tabstop=2 softtabstop=2 shiftwidth=2
