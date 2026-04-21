#include <windows.h>
#include <cstdint>



// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam) {
	// メッセージに応じてゲーム固有の処理を行う

	switch (msg) {
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準 のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);

}

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
WNDCLASS wc{};

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
	
	// クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	// クライアント領域をもとに実際のサイズにwrcに変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

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

	ShowWindow(hwnd, SW_SHOW);

	MSG msg{};
	//ウィンドウボタンの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT){

		// Windowにメッセージが来ていたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//ゲームの処理
		}


	}
	return 0;
}
