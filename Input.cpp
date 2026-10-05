#include "Input.h"
#include <cassert>

void  Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT hr;

	// 初期化処理

	hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);

	hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(hr));

	hr = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));


}

bool Input::PushKey(BYTE keyNumber)
{
	// 指定キーを押していればtrueを返す
	if (key[keyNumber]) {
		return true;
	}

	return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	// 指定キーを離していればtrueを返す
	if (!key[keyNumber]) {
		return true;
	}
	return false;
}

void Input::Updata()
{
	// 更新処理

	HRESULT hr;

	// 前回のキーの入力状態を保存
	memcpy(keyPre, key, sizeof(key));

	// キーボード情報の取得開始
	hr = keyboard->Acquire();
	hr = keyboard->GetDeviceState(sizeof(key), key);


}