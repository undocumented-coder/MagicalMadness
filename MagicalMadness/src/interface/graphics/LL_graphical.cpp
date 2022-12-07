#include <iostream>

#include "LL_graphical.hpp"

#include "dependencies/imgui/imgui.h"
#include "dependencies/imgui/imgui_impl_win32.h"
#include "dependencies/imgui/imgui_impl_dx11.h"

LL_graphical_t::~LL_graphical_t()
{
	std::printf("LL_graphical deconstructing ImGui & DirectX11!\n");

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (this->p_render_target)
		this->p_render_target->Release();

	if (this->p_swapchain)
		this->p_swapchain->Release();

	if (this->p_context)
		this->p_context->Release();

	if (this->p_device)
		this->p_device->Release();
}

void LL_graphical_t::setup(HWND hwnd)
{
	this->parent_window = hwnd;

	this->create_device_and_swapchain();
	this->create_render_target();
	this->initialize_imgui();

	std::printf("Device: 0x%p\nContext: 0x%p\nSwap Chain: 0x%p\nRender Target: 0x%p\n",
		this->p_device,
		this->p_context,
		this->p_swapchain,
		this->p_render_target);
}

void LL_graphical_t::create_device_and_swapchain()
{
	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

	RECT dims{};
	GetClientRect(this->parent_window, &dims);

	DXGI_SWAP_CHAIN_DESC descriptor{};
	descriptor.BufferDesc.Width = 0;
	descriptor.BufferDesc.Height = 0; // "If you specify the height as zero when you call the IDXGIFactory::CreateSwapChain method to create a swap chain, the runtime obtains the height from the output window and assigns this height value to the swap-chain description"
	descriptor.BufferDesc.RefreshRate.Numerator = 60;
	descriptor.BufferDesc.RefreshRate.Denominator = 1;
	descriptor.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descriptor.SampleDesc.Count = 1;
	descriptor.SampleDesc.Quality = 0;
	descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	descriptor.BufferCount = 2;
	descriptor.OutputWindow = this->parent_window;
	descriptor.Windowed = TRUE;
	descriptor.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	descriptor.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, levels, sizeof(levels) / sizeof(levels[0]), D3D11_SDK_VERSION, &descriptor, &this->p_swapchain, &this->p_device, NULL, &this->p_context);
	if (res != S_OK)
	{
		std::printf("Error with creating D3D11 interface! 0x%p\n", res);
		return;
	}
}

void LL_graphical_t::create_render_target()
{
	ID3D11Texture2D* p_backbuffer = nullptr;
	this->p_swapchain->GetBuffer(0, __uuidof(p_backbuffer), reinterpret_cast<void**>(&p_backbuffer)); // "If the swap chain's swap effect is DXGI_SWAP_EFFECT_DISCARD, this method can only access the first buffer; for this situation, set the index to zero."
		this->p_device->CreateRenderTargetView(p_backbuffer, nullptr, &this->p_render_target);
	p_backbuffer->Release();
}

void LL_graphical_t::internal_render()
{
	ImGui::Render();
	const float clear_color_with_alpha[4] = {0.f, 0.f, 0.f, 1.f};
	this->p_context->OMSetRenderTargets(1, &this->p_render_target, NULL);
	this->p_context->ClearRenderTargetView(this->p_render_target, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	this->p_swapchain->Present(0, 0); // Present without vsync (1,0) = vsync
}

void LL_graphical_t::initialize_imgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplWin32_Init(this->parent_window);
	ImGui_ImplDX11_Init(this->p_device, this->p_context);
}