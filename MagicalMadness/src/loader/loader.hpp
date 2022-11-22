#pragma once
#include <Windows.h>
#include <string>
#include <utility>

// The loader will be responsible for opening the file and reading PE information about it.

class loader_t
{
private:
	std::string file_path;
	HANDLE h_file = INVALID_HANDLE_VALUE;
	HANDLE h_map = INVALID_HANDLE_VALUE;
	void* map_base_address = nullptr;

	template <typename T>
	T get_image_directory_address(PIMAGE_NT_HEADERS32 pe_header, std::uint32_t image_directory);
public:
	loader_t(const std::string& file_path) : file_path{ file_path } {};
	~loader_t();

	void analyze();
};