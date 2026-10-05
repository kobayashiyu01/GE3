#pragma once
#include "windows.h"
#include <wrl.h>
#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


class Input
{
public:
	// namespace省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;



public: // メンバ関数

	// 初期化
	void Initialize(HINSTANCE hInstance, HWND hwnd);

	// 更新
	void Update();

	// キーの押下を判定する関数
	bool PushKey(BYTE keyNumber);

	// キーの離上を判定する関数
	bool TriggerKey(BYTE keyNumber);

private:

	// DirectInputの初期化
	ComPtr<IDirectInput8> directInput;

	// キーボードのデバイス
	ComPtr<IDirectInputDevice8> keyboard; // キーボード 

	// 全キーの入力状態を取得する
	BYTE key[256] = {};

	// 前回のキーの入力状態を取得する
	BYTE keyPre[256] = {};

};

