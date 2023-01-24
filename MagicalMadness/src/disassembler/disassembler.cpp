#define ZYDIS_STATIC_BUILD

#include "disassembler.hpp"
#include <Zydis/Disassembler.h>
#include <zydis/SharedTypes.h>

// Thanks Zydis for making this simple
std::string& disassembler_t::disassemble(std::uint32_t loaded_base)
{
	this->disassembled = "";
	if (this->bounds.has_read != true || this->bounds.is_code != true)
		return this->disassembled;

	char formatted_text[100]{0}; // more than enough for no stack corrupt
	std::uint32_t current_ptr = loaded_base + this->bounds.start_address;
	while (current_ptr < loaded_base + this->bounds.end_address)
	{
		ZydisDisassembledInstruction instruction{};
		ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LEGACY_32, loaded_base, reinterpret_cast<void*>(current_ptr), 50, &instruction);
		sprintf_s(formatted_text, "[0x%p]: %s\n", current_ptr, instruction.text);
		this->disassembled += formatted_text;
		std::memset(formatted_text, 0, 100);
		current_ptr += (instruction.info.length ? instruction.info.length : 1);
	}

	return this->disassembled;
}

std::string& disassembler_t::get_previous_disassembly()
{
	return this->disassembled;
}