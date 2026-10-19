#pragma once
#include <windows.h>
#include <cstdint>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#pragma comment(lib,"dxcompiler.lib")
#endif 
class WinApp
{
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


public:
	WNDCLASS wc{};
	void Initialize();

	void Update();

private:

	// クライアント領域のサイズ
	int32_t kClientWidth = 1280;
	int32_t kClientHeight = 720;

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };


	//ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,         // クラス名
		L"CG2",                   // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,      // ウィンドウスタイル
		CW_USEDEFAULT,            // X座標
		CW_USEDEFAULT,            // Y座標
		wrc.right - wrc.left,     // ウィンドウ横幅
		wrc.bottom - wrc.top,     // ウィンドウ縦幅
		nullptr,                  // 親ウィンドウハンドル
		nullptr,                  // メニューハンドル
		wc.hInstance,             // インスタンスハンドル
		nullptr);                 // オプション


};

