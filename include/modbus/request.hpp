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
#ifndef MODBUS_REQUEST_H_
#define MODBUS_REQUEST_H_

#include <cstdint>
#include <vector>

#include "functions.hpp"

namespace modbus {

namespace response {
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

namespace request {
    struct request {
		std::uint8_t function = 0;
		std::uint16_t address;
    };

	/// Message representing a read_coils request.
	struct read_coils {
		/// Response type.
		using response = response::read_coils;

		/// The function code.
		static constexpr std::uint8_t function = functions::read_coils;

		/// The address of the first coil/register to read from.
		std::uint16_t address;

		/// The number of registers/coils to read.
		std::uint16_t count;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a read_discrete_inputs request.
	struct read_discrete_inputs {
		/// Response type.
		using response = response::read_discrete_inputs;

		/// The function code.
		static constexpr std::uint8_t function = functions::read_discrete_inputs;

		/// The address of the first coil/register to read from.
		std::uint16_t address;

		/// The number of registers/coils to read.
		std::uint16_t count;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a read_holding_registers request.
	struct read_holding_registers {
		/// Response type.
		using response = response::read_holding_registers;

		/// The function code.
		static constexpr std::uint8_t function = functions::read_holding_registers;

		/// The address of the first coil/register to read from.
		std::uint16_t address;

		/// The number of registers/coils to read.
		std::uint16_t count;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a read_input_registers request.
	struct read_input_registers {
		/// Response type.
		using response = response::read_input_registers;

		/// The function code.
		static constexpr std::uint8_t function = functions::read_input_registers;

		/// The address of the first coil/register to read from.
		std::uint16_t address;

		/// The number of registers/coils to read.
		std::uint16_t count;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a write_single_coil request.
	struct write_single_coil {
		/// Response type.
		using response = response::write_single_coil;

		/// The function code.
		static constexpr std::uint8_t function = functions::write_single_coil;

		/// The address of the coil to write to.
		std::uint16_t address;

		/// The value to write.
		bool value;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a write_single_register request.
	struct write_single_register {
		/// Response type.
		using response = response::write_single_register;

		/// The function code.
		static constexpr std::uint8_t function = functions::write_single_register;

		/// The address of the register to write to.
		std::uint16_t address;

		/// The value to write.
		std::uint16_t value;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 5;
		}
	};

	/// Message representing a write_multiple_coils request.
	struct write_multiple_coils {
		/// Response type.
		using response = response::write_multiple_coils;

		/// The function code.
		static constexpr std::uint8_t function = functions::write_multiple_coils;

		/// The address of the first coil to write to.
		std::uint16_t address;

		/// The values to write.
		std::vector<bool> values;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 6 + (values.size() + 7) / 8;
		}
	};

	/// Message representing a write_multiple_registers request.
	struct write_multiple_registers {
		/// Response type.
		using response = response::write_multiple_registers;

		/// The function code.
		static constexpr std::uint8_t function = functions::write_multiple_registers;

		/// The address of the first register to write to.
		std::uint16_t address;

		/// The values to write.
		std::vector<std::uint16_t> values;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 6 + values.size() * 2;
		}
	};

	/// Message representing a mask_write_register request.
	struct mask_write_register {
		/// Response type.
		using response = response::mask_write_register;

		/// The function code.
		static constexpr std::uint8_t function = functions::mask_write_register;

		/// The address of the register to write to.
		std::uint16_t address;

		/// The mask to AND the register value with.
		std::uint16_t and_mask;

		/// The mask to OR the register value with.
		std::uint16_t or_mask;

		/// The length of the serialized PDU in bytes.
		std::size_t length() const {
			return 7;
		}
	};

}}

#endif
// vim: autoindent syntax=cpp expandtab tabstop=2 softtabstop=2 shiftwidth=2
