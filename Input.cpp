#include "Input.h"
#include <cassert>

void  Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT hr;

	// 初期化処理
	// DirectInputの初期化
	ComPtr<IDirectInput8> directInput;

	hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);

	hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(hr));

	hr = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));


}

void Input::Updata()
{
	// 更新処理
	// キーボード情報の取得開始
	keyboard->Acquire();

	// 全キーの入力状態を取得する
	BYTE key[256] = {};
	keyboard->GetDeviceState(sizeof(key), key);


}