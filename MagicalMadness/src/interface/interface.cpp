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

void interface_t::render(const loader_output_t& information) const
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	if (window_open)
	{
		ImGui::Begin("PE Information", &window_open);

			ImGui::Text("Successful disassembly: %s\n", information.successful ? "yes" : "no");
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
	}
	else
		PostQuitMessage(0);

	this->graphics->internal_render();
}