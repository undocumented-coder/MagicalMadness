#pragma once
#include <Windows.h>
#include <string_view>
#include <memory>

#include "graphics/LL_graphical.hpp"


// Holds higher level window interface code (Will eventually write this to act as a sort of interface, for now its just a window).
class interface_t
{
private:
	const std::string_view window_title = "";
	mutable std::string window_class_name = "";
	mutable HWND h_wnd = nullptr;
	std::unique_ptr<LL_graphical_t> graphics = std::make_unique<LL_graphical_t>(); // holds DirectX11 data for ImGui
public:
	interface_t();
	interface_t(std::string_view title);
	interface_t(const interface_t&) = delete;
	~interface_t();


	void initialize() const;
	void render() const;
	bool messenger() const;
};