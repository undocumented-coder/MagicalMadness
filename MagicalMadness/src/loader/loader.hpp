#pragma once
#include <Windows.h>
#include <string>
#include <utility>
#include <vector>

// The loader will be responsible for opening the file and reading PE information about it.

struct section_t
{
	std::string section_name = "";
	std::uint8_t is_code, is_data, has_read, has_write, has_execute = false;

	// All addresses are virtual not mapped
	std::uint32_t start_address = 0; 
	std::uint32_t end_address = 0;
	std::uint32_t pointer_raw_data = 0;
	std::uint32_t raw_data_size = 0;

	section_t(const char* name, std::uint32_t start, std::uint32_t end, std::uint32_t raw_data_start, std::uint32_t raw_data_size, std::uint32_t flags) :
		section_name{ name }, start_address{ start }, end_address{ end }, pointer_raw_data{raw_data_start}, raw_data_size{ raw_data_size }
	{
		is_code = (flags & IMAGE_SCN_CNT_CODE) == IMAGE_SCN_CNT_CODE;
		is_data = ((flags & IMAGE_SCN_CNT_INITIALIZED_DATA) == IMAGE_SCN_CNT_INITIALIZED_DATA) || ((flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == IMAGE_SCN_CNT_UNINITIALIZED_DATA);
		has_execute = (flags & IMAGE_SCN_MEM_EXECUTE) == IMAGE_SCN_MEM_EXECUTE;
		has_read = (flags & IMAGE_SCN_MEM_READ) == IMAGE_SCN_MEM_READ;
		has_write = (flags & IMAGE_SCN_MEM_WRITE) == IMAGE_SCN_MEM_WRITE;
	};
};

class loader_t
{
private:
	std::string file_path{};
	HANDLE h_file = INVALID_HANDLE_VALUE;
	HANDLE h_map = INVALID_HANDLE_VALUE;
	void* map_base_address = nullptr;
	std::vector<section_t> sections{};

	template <typename T>
	T get_image_directory_address(PIMAGE_NT_HEADERS32 pe_header, std::uint32_t image_directory);
	void extract_sections_from_PE(PIMAGE_NT_HEADERS32 pe_header);
public:
	loader_t(const std::string& file_path) : file_path{ file_path } {};
	~loader_t();

	void analyze();
	void testing();
};