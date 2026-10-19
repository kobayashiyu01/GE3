#include "WinApp.h"

LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
	// メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
	case WM_DESTROY: // ウィンドウが破棄された
		PostQuitMessage(0); // OSに対してアプリの終了と伝える
		return 0;
	}
	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WinApp::Initialize()
{

	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウクラスの名前
	wc.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウィンドウクラスを登録する
	RegisterClass(&wc);

	// クライアント領域をもとに実際のサイズにwrcに変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウの生成
	hwnd = CreateWindow(
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

	ShowWindow(hwnd, SW_SHOW);
}

bool WinApp::ProcessMessage()
{
	MSG msg{};
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT) 
	{
		return true;
	}

	return false;
}

void WinApp::Update()
{}

void WinApp::Finalize()
{
	CloseWindow(hwnd);
	CoUninitialize();
}
