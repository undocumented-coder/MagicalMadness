#include <Windows.h>
#include <iostream>

#include "loader/loader.hpp"

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
	create_console();

	std::printf("Welcome to Magical Madness!\n");

	if (argc != 2) // First argument is it's own file path.
	{
		std::printf("Please try to run MagicalMadness.exe with the file you'd like to analyze!\n");
	}
	else
	{
		loader_t executable{ argv[1] };
		executable.analyze();
	}

	while (true) {};

	return 1;
}