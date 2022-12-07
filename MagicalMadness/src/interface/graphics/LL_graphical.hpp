#pragma once
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

extern class interface_t;

// holds low level graphics engine code
class LL_graphical_t
{
private:
	friend class interface_t;
	HWND parent_window = nullptr;
	ID3D11Device* p_device = nullptr;
	ID3D11DeviceContext* p_context = nullptr;
	ID3D11RenderTargetView* p_render_target = nullptr;
	IDXGISwapChain* p_swapchain = nullptr;

	void create_device_and_swapchain();
	void create_render_target();
	void initialize_imgui();
	void internal_render();
public:
	LL_graphical_t() = default;
	~LL_graphical_t();

	void setup(HWND hwnd);
};