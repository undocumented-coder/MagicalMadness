#pragma once
#include <iostream>
#include <cstdint>

#include "loader/loader.hpp"

class disassembler_t
{
private:
	std::string disassembled{};
	section_t bounds;
public:
	disassembler_t(section_t section) : bounds{ section } {};
	disassembler_t(const disassembler_t&) = delete;

	std::string& disassemble(std::uint32_t loaded_base);
	std::string& get_previous_disassembly();
};