#include <iostream>
#include "loader.hpp"
#include "disassembler/disassembler.hpp"

// Disclaimer:
// This code uses LOTS of undocumented windows internals. They won't be just there documented for you, it took time to reverse engineering the
// underlying windows components and slowly piecing together this work.


void print_error()
{
	LPSTR ptr_msg = nullptr;

	int n_bytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), NULL, reinterpret_cast<LPSTR>(&ptr_msg), NULL, NULL);
	if (!n_bytes)
	{
		std::printf("[Error]: Error in error handling, raw error code: 0x%p\n", GetLastError());
	}
	else
	{
		std::printf("[Error]: %s", ptr_msg);
	}
}

loader_t::~loader_t()
{
	std::printf("Unmapping files!\n");

	if (this->map_base_address != nullptr)
		UnmapViewOfFile(this->map_base_address);
	if (this->h_map != INVALID_HANDLE_VALUE)
		CloseHandle(this->h_map);
	if (this->h_file != INVALID_HANDLE_VALUE)
		CloseHandle(this->h_file);
}

void loader_t::extract_sections_from_PE(PIMAGE_NT_HEADERS32 pe_header)
{
	this->sections.clear();
	PIMAGE_SECTION_HEADER first_section = IMAGE_FIRST_SECTION(pe_header);
	for (std::uint32_t a = 0; a < pe_header->FileHeader.NumberOfSections; ++a)
	{
		PIMAGE_SECTION_HEADER current_section = first_section + a;
		this->sections.emplace_back(reinterpret_cast<const char*>(current_section->Name), current_section->VirtualAddress, current_section->VirtualAddress + current_section->Misc.VirtualSize, current_section->PointerToRawData, current_section->SizeOfRawData, current_section->Characteristics);
	}
}

template<typename T>
T loader_t::get_image_directory_address(PIMAGE_NT_HEADERS32 pe_header, std::uint32_t data_directory_id)
{
	if (pe_header->OptionalHeader.NumberOfRvaAndSizes <= data_directory_id)
	{
		std::printf("[Error]: Data directory out of bounds.\n");
		return reinterpret_cast<T>(nullptr);
	}

	IMAGE_DATA_DIRECTORY data_directory = pe_header->OptionalHeader.DataDirectory[data_directory_id];



	std::printf("[Error]: Failed to find base of data directory.\n");
	return reinterpret_cast<T>(nullptr);
}

void append_to_output(std::string& output, const char* text)
{
	output += text;
}

void append_to_output(std::string& output, const char* formatting, auto... args)
{
	char buffer[1024]{ 0 };
	sprintf_s(buffer, formatting, args...);

	output += buffer;
}


// A complete guide to the internals of the windows PE format: http://www.csn.ul.ie/~caolan/pub/winresdump/winresdump/doc/pefile2.html
void loader_t::analyze(loader_output_t& loader_output)
{
	std::string& output = loader_output.output;

	append_to_output(output, "Analyzing: %s\n", this->file_path.substr(this->file_path.find_last_of("\\") + 1).c_str());
	this->h_file = CreateFile(this->file_path.c_str(), GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (this->h_file == INVALID_HANDLE_VALUE)
	{
		print_error();
		return;
	}

	// File mapping is a tricky concept, here's it briefly explained from: https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping
	// "allows the process to work efficiently with a large data file, such as a database, without having to map the whole file into memory"
	// Also: https://learn.microsoft.com/en-us/windows/win32/memory/creating-a-file-mapping-object
	
	// Remove SEC_IMAGE if you want to be able to load images which are maybe scrambled up a bit (such as hiding section names)
	this->h_map = CreateFileMapping(this->h_file, NULL, PAGE_READONLY | SEC_IMAGE, NULL, NULL, NULL);
	
	void* base_address = MapViewOfFile(this->h_map, FILE_MAP_READ, NULL, NULL, NULL);
	this->map_base_address = base_address;

	if (base_address != NULL)
	{
		append_to_output(output, "Successfully mapped process into address: 0x%p\n", base_address);
		PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(base_address);

		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		{
			append_to_output(output, "Program doesn't have a DOS header (signature: 0x%04X)\n", dos_header->e_magic);
			return;
		}

		if (!dos_header->e_lfanew)
		{
			append_to_output(output, "Program doesn't have a PE header (probably just a DOS program).\n");
			return;
		}

		PIMAGE_NT_HEADERS32 pe_header = reinterpret_cast<PIMAGE_NT_HEADERS32>(reinterpret_cast<std::uint32_t>(base_address) + dos_header->e_lfanew);
		if (pe_header->Signature != IMAGE_NT_SIGNATURE)
		{
			append_to_output(output, "Program is not a valid PE file! (Possibly just a DOS program)\n");
			return;
		}

		if (pe_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		{
			if (pe_header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			{
				append_to_output(output, "This file is an x64 file, this program can only process x86.\n");
				return;
			}

			append_to_output(output, "Unknown file architecture.\n");
			return;
		}

		std::uint16_t PE_behavior = pe_header->FileHeader.Characteristics;
		if (!(PE_behavior & IMAGE_FILE_EXECUTABLE_IMAGE) && !(PE_behavior & IMAGE_FILE_32BIT_MACHINE))
		{
			append_to_output(output, "This file is not executable and/or it's not x86.\n");
			return;
		}

		if (pe_header->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
		{
			append_to_output(output, "The disassembler only supports I386 files, it may be inaccurate due to your file being the wrong machine.\n");
		}

		std::uint32_t image_base = pe_header->OptionalHeader.ImageBase;
		std::uint32_t entry_point = pe_header->OptionalHeader.AddressOfEntryPoint;

		append_to_output(output, "Executable information:\n\tImage Base: 0x%p\n\tEntry Point: 0x%p\n", image_base, entry_point);

		append_to_output(output, "Sections:\n");
		PIMAGE_SECTION_HEADER first_section = IMAGE_FIRST_SECTION(pe_header);
		for (std::uint32_t i = 0; i < pe_header->FileHeader.NumberOfSections; ++i)
		{
			PIMAGE_SECTION_HEADER current_section = first_section + i;
			bool contains_code = current_section->Characteristics & IMAGE_SCN_CNT_CODE;
			
			std::string permissions = "";

			if (current_section->Characteristics & IMAGE_SCN_MEM_READ)
				permissions += " [READ]";
			if (current_section->Characteristics & IMAGE_SCN_MEM_WRITE)
				permissions += " [WRITE]";
			if (current_section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
				permissions += " [EXECUTE]";

			append_to_output(output, "\t[%s]: [RVA: 0x%p, Size: 0x%p] %s %s\n", current_section->Name, current_section->VirtualAddress, 
				current_section->Misc.VirtualSize, contains_code ? "[CODE SECTION]" : "[DATA SECTION]", permissions.c_str());
		}
		this->extract_sections_from_PE(pe_header);

		std::uint32_t base_addy = reinterpret_cast<std::uint32_t>(base_address); // lots of this code below needs this as a number


		PIMAGE_EXPORT_DIRECTORY export_directory = this->get_image_directory_address<PIMAGE_EXPORT_DIRECTORY>(pe_header, IMAGE_DIRECTORY_ENTRY_EXPORT);

		if (export_directory)
		{
			append_to_output(output, "Exports:\n");
			append_to_output(output, "\tFile internal name: %s\n", reinterpret_cast<const char*>(base_addy + export_directory->Name));
			
			std::uint32_t* export_names = reinterpret_cast<std::uint32_t*>(base_addy + export_directory->AddressOfNames);
			std::uint16_t* export_ordinals = reinterpret_cast<std::uint16_t*>(base_addy + export_directory->AddressOfNameOrdinals); // just holds an ID, which you can look up for function index
			std::uint32_t* export_functions = reinterpret_cast<std::uint32_t*>(base_addy + export_directory->AddressOfFunctions);

			for (std::uint32_t i = 0; i < export_directory->NumberOfNames; ++i)
			{
				append_to_output(output, "\tLocated export: %s - 0x%p\n", reinterpret_cast<const char*>(base_addy + export_names[i]), export_functions[export_ordinals[i]]);
			}
		}
		else
		{
			append_to_output(output, "no exports\n");
		}

		PIMAGE_IMPORT_DESCRIPTOR current_import = this->get_image_directory_address<PIMAGE_IMPORT_DESCRIPTOR>(pe_header, IMAGE_DIRECTORY_ENTRY_IMPORT);

		if (current_import)
		{
			append_to_output(output, "Imports:\n");
			while (current_import->Name)
			{
				append_to_output(output, "[ %s ]\n", reinterpret_cast<const char*>(base_addy + current_import->Name));

				PIMAGE_THUNK_DATA32 current_thunk = reinterpret_cast<PIMAGE_THUNK_DATA32>(base_addy + current_import->FirstThunk);
				while (current_thunk->u1.AddressOfData)
				{
					// ordinal only
					if (current_thunk->u1.AddressOfData & IMAGE_ORDINAL_FLAG32)
					{
						append_to_output(output, "\tOrdinal: %d\n", current_thunk->u1.AddressOfData ^ IMAGE_ORDINAL_FLAG32);
					}
					else
					{
						PIMAGE_IMPORT_BY_NAME import_name = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(base_addy + current_thunk->u1.AddressOfData);
						append_to_output(output, "\t%s\n", import_name->Name);
					}


					++current_thunk;
				}

				++current_import;
			}
		}
		else
		{
			append_to_output(output, "no imports (???)\n");
		}


		extract_sections_from_PE(pe_header);
		for (const section_t& section : this->sections)
		{
			if (section.has_read && section.is_code)
			{
				disassembler_t disassembler{ section };
				loader_output.disassembled_code[section.section_name] = disassembler.disassemble(reinterpret_cast<std::uint32_t>(this->map_base_address));
			}
		}
	}
	else
	{
		append_to_output(output, "[Error]: Failed to map file into memory!\n");
		print_error();
		append_to_output(output, "This could have occured due to you giving MagicalMadness a file that isn't a .exe file format.\n");
		return;
	}

	loader_output.successful = true;
	return;
}