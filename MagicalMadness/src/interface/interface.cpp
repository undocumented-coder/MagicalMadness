#include <time.h>
#include <sstream>
#include <dwmapi.h>

#include "interface.hpp"
#include "dependencies/imgui/imgui.h"
#include "dependencies/imgui/imgui_impl_win32.h"
#include "dependencies/imgui/imgui_impl_dx11.h"
#include <dependencies/imgui/imgui_internal.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI custom_message_handler(
	_In_ HWND hWnd,
	_In_ UINT msg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam)
{
	// ImGui internal message.
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
		case WM_SIZE: // not supported right now (would crash)
			std::printf("WM_SIZE attempt!\n");
			return 0;
		case WM_DESTROY: // When a window is destroyed (WM_CLOSE in default window proc destroys the window, which will hit this message)
			PostQuitMessage(0);
			break;
		default:
			break;
	}

	return DefWindowProcA(hWnd, msg, wParam, lParam);
}

interface_t::interface_t()
{
	this->initialize();
}

interface_t::interface_t(std::string_view title) : window_title{ title }
{
	this->initialize();
}

void interface_t::initialize() const
{
	srand(time(NULL));

	if (!this->window_title.size())
	{
		std::stringstream ss{};
		ss << "{MADNESS_" << std::hex << rand() * rand() << "}";
		this->window_class_name = ss.str();
	}
	else
		(this->window_class_name += (this->window_title)) += "_wndclass";

	WNDCLASSEXA wnd_class{};
	wnd_class.cbSize = sizeof(WNDCLASSEXA);
	wnd_class.style = CS_CLASSDC;
	wnd_class.lpfnWndProc = custom_message_handler;
	wnd_class.lpszClassName = this->window_class_name.c_str();
	wnd_class.hInstance = GetModuleHandleA(NULL);

	ATOM atom = RegisterClassExA(&wnd_class);

	RECT output{};
	GetWindowRect(GetDesktopWindow(), &output);

	this->h_wnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, wnd_class.lpszClassName, this->window_title.data(),
		WS_POPUP | WS_VISIBLE, 0, 0, 1, 1, nullptr, nullptr, wnd_class.hInstance, nullptr); // Create transparent see through window

	SetLayeredWindowAttributes(this->h_wnd, RGB(0, 0, 0), 1.f, LWA_ALPHA); // All black pixels will pass through as transparent (LWA_COLORKEY is INSANELY slow)

	MARGINS margin = { -1 };
	DwmExtendFrameIntoClientArea(this->h_wnd, &margin);

	UpdateWindow(this->h_wnd);
	SetWindowPos(this->h_wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// Setup DirectX11 & ImGui
	this->graphics->setup(this->h_wnd);

	std::printf("Successfully initialized interface!\n");
}

interface_t::~interface_t()
{
	DestroyWindow(this->h_wnd);
	UnregisterClassA(this->window_class_name.c_str(), nullptr);

	std::printf("Deconstructed interface!\n");
}

bool interface_t::messenger() const
{
	MSG msg;
	while (PeekMessageA(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
		if (msg.message == WM_QUIT)
			return true;
	}

	return false;
}

static bool window_open = true;


#include "compiler/interpreter/include/interpreter.hpp"
#include "compiler/parser/parser.hpp"


std::string script_buffer{};

static bool is_init = false;
void initialize_script_buffer()
{
	if (is_init)
		return;

	is_init = true;
	script_buffer.resize(15000, '\0'); // Script box can hold 15k chars
}

std::int32_t HelloComputer()
{
	std::printf("Hello I am on the C++ side! This went through the scripting languages interpreter into a native C++ call, to here.\n");
	return 1;
}

void run_compiler_script()
{
	static bool warn = false;
	if (!warn)
	{
		warn = true;
		std::printf("[WARNING]: Error messages are bad - not fully added yet; most will come with semantic analysis pass once I add it.\n");
	}

	static std::shared_ptr<environment_t> global_environment = std::make_shared<environment_t>(); // Keep environment static so it remembers.

	std::shared_ptr<runtime_function_t> debug_function = std::make_shared<runtime_function_t>("HelloComputer", HelloComputer); // Expose C++ function to my language
	global_environment->assign("HelloComputer", debug_function);

	try
	{
		std::unique_ptr<parser_t> parser = std::make_unique<parser_t>(script_buffer);
		std::unique_ptr<program_t> program = parser->parse();

		interpreter::run_tree(std::move(program), global_environment);
	}
	catch (std::exception& err)
	{
		std::printf("Parsing failed!\n%s\n", err.what());
	}

	std::printf("Script ran successfully!\n");
}

void interface_t::render(const loader_output_t& information) const
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (window_open)
	{
		ImGuiContext* ctx = ImGui::GetCurrentContext();

		ctx->Style.Colors[ImGuiCol_WindowBg] = ImColor{ 53, 53, 53 };
		ctx->Style.Colors[ImGuiCol_TitleBgActive] = ImColor{ 229, 0, 95 };
		ctx->Style.Colors[ImGuiCol_TitleBg] = ImColor{ 104, 0, 43 };
		ctx->Style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor{ 104, 0, 43 };
		ctx->Style.Colors[ImGuiCol_ResizeGrip] = ImColor{ 255, 73, 122 };
		ctx->Style.Colors[ImGuiCol_ResizeGripHovered] = ImColor{ 255, 20, 75 };
		ctx->Style.Colors[ImGuiCol_ResizeGripActive] = ImColor{ 255, 20, 75 };
		//ctx->Style.Colors[]
		
		ImGui::Begin("PE Information", &window_open);
			ImVec2 sz = ImGui::GetWindowSize();
			ImGui::SetCursorPos({ sz.x / 2 - sz.x / 8, 20 });
			ImGui::Text("Successful disassembly: %s\n", information.successful ? "yes" : "no");
			ImGui::SetCursorPos({ 20, sz.y / 2 });
			ImGui::TextUnformatted(information.output.c_str(), &*information.output.end());

		ImGui::End();

		ImGui::Begin("Disassembled Code", &window_open);

			for (auto& [section, disassembly] : information.disassembled_code)
			{
				if (ImGui::TreeNode(section.c_str()))
				{
					ImGui::TextUnformatted(disassembly.c_str(), &*disassembly.end());
					ImGui::TreePop();
				}
			}

		ImGui::End();

		initialize_script_buffer();
		ImGui::Begin("Scripting Suite", &window_open);
			ImVec2 window_size = ImGui::GetWindowSize();

			ImGui::Text("This all runs on a completely custom compiler.\nInsert a script below, output will show in the C++ console.\nThis compiler supports operator precedence, unary, negate, variables and native functions. (C++ invoke)\nEach line will be an output.\nExample script for computing a jump table:\n\nSomeValue = 0x401000; JumpIndex = 5; SomeValue + JumpIndex * 4;\n\nAn example for calling C++ is below (and in interface.cpp):\n\nHelloComputer();");
			ImGui::InputTextMultiline("", &script_buffer[0], script_buffer.size(), { window_size.x - 25.f, window_size.y - 210.f }, ImGuiInputTextFlags_AllowTabInput);
			if (ImGui::Button("Run Script"))
			{
				run_compiler_script();
				std::memset(&script_buffer[0], '\0', script_buffer.size());
			}
		ImGui::End();
	}
	else
		PostQuitMessage(0);

	this->graphics->internal_render();
}