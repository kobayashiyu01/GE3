#include "windows.h"
#include <wrl.h>
#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#pragma once
class Input
{
public:
	// namespace省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;



public: // メンバ関数

	// 初期化
	void Initialize(HINSTANCE hInstance, HWND hwnd);

	// 更新
	void Updata();

private:

	// キーボードのデバイス
	ComPtr<IDirectInputDevice8> keyboard; // キーボード 
};

