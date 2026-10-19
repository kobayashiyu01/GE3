#pragma once
#include <windows.h>
#include <cstdint>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif 
class WinApp
{
public:
	
	WNDCLASS wc{};
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	// クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

public:
	
	void Initialize();

	void Update();
	
	void Finalize();

	HWND GetHwnd()const { return hwnd; }

	HINSTANCE GetHInstance() const { return wc.hInstance; }

	bool ProcessMessage();

private:

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	//ウィンドウの生成
	HWND hwnd = nullptr;


};

