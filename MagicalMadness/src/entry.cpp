#include <Windows.h>
#include <iostream>
#include <memory>

#include "loader/loader.hpp"
#include "interface/interface.hpp"

void create_console()
{
	FILE* f_handles[3]{ 0 };
	
	AllocConsole();
	freopen_s(&f_handles[0], "CONOUT$", "w", stdout);
	freopen_s(&f_handles[1], "CONOUT$", "w", stderr);
	freopen_s(&f_handles[2], "CONIN$", "r", stdin);

	SetConsoleTitleA("Magical Madness | Disassember & Reverse Engineering Tool");
}

// Makes it easier to identify where errors occured in this app.
LONG custom_exception_handler(PEXCEPTION_POINTERS exception_info)
{
	std::printf("Module base: 0x%p\n", reinterpret_cast<std::uint32_t>(GetModuleHandle(NULL)));
	std::printf("Exception occured at: 0x%p. Error code: 0x%p.\n", exception_info->ExceptionRecord->ExceptionAddress, exception_info->ExceptionRecord->ExceptionCode);
	std::cin.get();

	return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char* argv[])
{
	std::printf("Welcome to Magical Madness!\n");
	
	// If any errors occur this will be the backbone which catches those errors and gives you a chance to see what went wrong (will make bug hunting MUCH easier)
	SetUnhandledExceptionFilter(reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(&custom_exception_handler));

	create_console();

	if (argc != 2) // First argument is it's own file path.
	{
		std::printf("Please try to run MagicalMadness.exe with the file you'd like to analyze!\n");
		std::printf("Running in test mode (no file given).\n");

		std::unique_ptr<interface_t> window = std::make_unique<interface_t>("Magical Madness");

		loader_output_t placeholder{};

		while (!window->messenger())
		{
			window->render(placeholder);
		}
	}
	else
	{
		std::unique_ptr<interface_t> window = std::make_unique<interface_t>("Magical Madness");

		loader_t executable{ argv[1] };
		loader_output_t output{};

		executable.analyze(output);

		while (!window->messenger())
		{
			window->render(output);
		}
	}
	
	std::printf("Shutting down!\n");

	return 1;
}