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


int main(int argc, char* argv[])
{
	std::printf("Welcome to Magical Madness!\n");
	create_console();

	if (argc != 2) // First argument is it's own file path.
	{
		std::printf("Please try to run MagicalMadness.exe with the file you'd like to analyze!\n");
	}
	else
	{
		std::unique_ptr<interface_t> window = std::make_unique<interface_t>("Magical Madness");

		loader_t executable{ argv[1] };
		executable.testing();

		while (!window->messenger())
		{
			window->render();
		}
	}


	std::printf("Shutting down!\n");

	return 1;
}