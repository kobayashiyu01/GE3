#include "DirectXCommon.h"
#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib,"dxgi.lib")


using namespace Microsoft::WRL;

static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;

}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
		descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	handleCPU.ptr += descriptorSize * index;

	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
		descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += descriptorSize * index;
	return handleGPU;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	//　ディスクリプタヒープの生成

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType; // レンダーターゲットビュー用
	descriptorHeapDesc.NumDescriptors = numDescriptors; // ダブルバッファ用に2つ。多くても別に構わない
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	// ディスクリプタヒープが作れなかったので起動できない
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateDepthStencilTextureResource(int32_t width, int32_t height)
{
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;                                   // Textureの幅
	resourceDesc.Height = height;                                 // textureの高さ
	resourceDesc.MipLevels =                                      // mipmapの数
		resourceDesc.DepthOrArraySize = 1;                            // 奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;          // DepthStencilとして使う通知
	resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// 深度値クリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;              // 1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる

	// Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,                  // Heapの設定
		D3D12_HEAP_FLAG_NONE,             // Heapの特殊な設定。特になし
		&resourceDesc,                    // Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値wokakiko状態にしておく
		&depthClearValue,                 // Clear最適値
		IID_PPV_ARGS(&resource));         // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	return resource;
}

void DirectXCommon::Initialize(WinApp* winApp)
{
	this->winApp = winApp;

	device = nullptr;
	dxgiFactory = nullptr;
	infoQueue = nullptr;
	commandAllocator = nullptr;
	commandList = nullptr;
	commandQueue = nullptr;
	swapChain = nullptr;
	fence = nullptr;
	rtvDescriptorHeap = nullptr;
	srvDescriptorHeap = nullptr;
	dsvDescriptorHeap = nullptr;
	depthstencilResource = nullptr;
	graphicsPipelineState = nullptr;
	dxcUtils = nullptr;
	dxcCompiler = nullptr;
	includehandler = nullptr;


	DeviceInitialize();
	CommandInitialize();
	SwapChainInitialize();
	DescriptorHeapInitialize();
	RenderTargetViewInitialize();
	DepthbufferInitialize();
	DepthStencilViewInitialize();
	FenceInitialize();
	ViewportInitialize();
	ScissorRectInitialize();
	DXCInitialize();
	ImGuiInitialize();
}
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index) {
	return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

void DirectXCommon::DeviceInitialize() {

	HRESULT hr;

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		// デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();

		// 更にGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);

	}
#endif

	// DXGIファクトリーの生成

	// HRESULTはWindows系のエラーコードであり、
	// 関数が成功したか失敗したかをSUCCEEDEDマクロで判定する
	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているかどうか、
	// どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	// 使用するアダプタ用の変数。最初にnullptrを入れておく。
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	// 良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter))
		!= DXGI_ERROR_NOT_FOUND; i++) {

		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));  // 取得できないのは一大事
		// ソフトウェアアダプタでなければ採用!
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプタの情報をログに出力。wstringのほうなので注意
			//ALog(ConvertString(std::format(L"Use     :{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;  // ソフトウェアアダプタの場合は見なかったことにする
	}

	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	// 昨機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};

	const char* featureLevelStrings[] = {
		"12.2",
		"12.1",
		"12.0"
	};

	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels);++i)
	{
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));

		// 指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr))
		{
			// 生成できたのでログに出力を行ってループを抜ける
			//ALog(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}

	// デバイスの生成がうまくいってなかったので起動できない
	assert(device != nullptr);
#ifdef _DEBUG


	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		// やばいエラー時にとまる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

		// エラー時にとまる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		// 警告時にとまる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {

			// Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			// https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		// 指定したメッセージの表示を抑制する
		infoQueue->AddStorageFilterEntries(&filter);

	}
# endif

}

void DirectXCommon::CommandInitialize() {

	HRESULT hr;

	// コマンドアロケータを生成する
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

	// コマンドロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドキューを生成する
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	// コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));
	
	// コマンドリストを生成する
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	// コマンドリストの生成がうまくいかなかったので起動できない
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

}

void DirectXCommon::SwapChainInitialize() {

	HRESULT hr;
	swapChainDesc.Width = WinApp::kClientWidth;   // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = WinApp::kClientHeight; // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2; // ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニタにうつしたら、中身を破棄

	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));

}

void DirectXCommon::DescriptorHeapInitialize() {

	descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	srvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

}

void DirectXCommon::RenderTargetViewInitialize() {
	// RTVの設定
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	// RTVのディスクリプタヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
		rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// バックバッファ2枚分のRTVを作成
	for (uint32_t i = 0; i < kRtvDescriptorCount; ++i) {

		// スワップチェーンからバックバッファを取得
		HRESULT hr = swapChain->GetBuffer(
			i,
			IID_PPV_ARGS(&swapChainResources[i])
		);
		assert(SUCCEEDED(hr));

		// RTVハンドルを取得
		rtvHandles[i].ptr =
			rtvStartHandle.ptr + descriptorSizeRTV * i;

		// レンダーターゲットビューを生成
		device->CreateRenderTargetView(
			swapChainResources[i].Get(),
			&rtvDesc,
			rtvHandles[i]
		);
	}

}

void DirectXCommon::DepthbufferInitialize() {

	//DepthStencilTextureをウィンドウのサイズで作成
	depthstencilResource = CreateDepthStencilTextureResource(
		WinApp::kClientWidth,
		WinApp::kClientHeight
	);

}

void DirectXCommon::DepthStencilViewInitialize()
{
	// DSVの設定
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	// DSVHeapの先頭ハンドルを取得
	dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// DSVを作成
	device->CreateDepthStencilView(
		depthstencilResource.Get(),
		&dsvDesc,
		dsvHandle
	);


}

void DirectXCommon::FenceInitialize()
{
	HRESULT hr;
	fenceValue = 0;

	// フェンスの生成
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	assert(SUCCEEDED(hr));

	// フェンスの完了を待つイベントを作成
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	assert(fenceEvent != nullptr);
}

void DirectXCommon::ViewportInitialize()
{
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = WinApp::kClientWidth;
	viewport.Height = WinApp::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void DirectXCommon::ScissorRectInitialize()
{
	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kClientHeight;
}

void DirectXCommon::DXCInitialize()
{
	HRESULT hr;

	// dxcCompilerを初期化
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	// 現時点でincludeはしないが、includeに対応するための設定を行っておく
	hr = dxcUtils->CreateDefaultIncludeHandler(&includehandler);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::ImGuiInitialize()
{
#ifdef USE_IMGUI
	// ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());
	ImGui_ImplDX12_Init(device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Build();
	io.MouseDelta;
#endif // USE_IMGUI
}

void DirectXCommon::Update()
{

}

void DirectXCommon::PreDraw()
{
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);
	ID3D12DescriptorHeap* descriptorHeaps[] = {
	srvDescriptorHeap.Get()
	};
	commandList->SetDescriptorHeaps(
		1,
		descriptorHeaps
	);

	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource =
		swapChainResources[backBufferIndex].Get();

	barrier.Transition.Subresource =
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	barrier.Transition.StateBefore =
		D3D12_RESOURCE_STATE_PRESENT;

	barrier.Transition.StateAfter =
		D3D12_RESOURCE_STATE_RENDER_TARGET;

	commandList->ResourceBarrier(1, &barrier);


	//-------------------------------------
	// DSV
	//-------------------------------------
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	//--------------------------------------
	// RTV設定
	//--------------------------------------
	commandList->OMSetRenderTargets(
		1,
		&rtvHandles[backBufferIndex],
		FALSE,
		&dsvHandle);

	//--------------------------------------
	// クリア
	//--------------------------------------

	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f }; // 青っぽい色。RGBAの順
	commandList->ClearRenderTargetView(
		rtvHandles[backBufferIndex],
		clearColor,
		0,
		nullptr);

	//--------------------------------------
	// 描画設定
	//--------------------------------------
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}

void DirectXCommon::PostDraw()
{
	// RENDER_TARGET → PRESENT
	barrier.Transition.StateBefore =
		D3D12_RESOURCE_STATE_RENDER_TARGET;

	barrier.Transition.StateAfter =
		D3D12_RESOURCE_STATE_PRESENT;

	commandList->ResourceBarrier(1, &barrier);

	// コマンドリストを閉じる
	HRESULT hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// コマンドリストを実行
	ID3D12CommandList* commandLists[] = {
		commandList.Get()
	};

	commandQueue->ExecuteCommandLists(
		1, commandLists
	);

	// 画面に表示
	swapChain->Present(1, 0);

	// Fenceの値を更新
	fenceValue++;

	// FenceをSignal
	commandQueue->Signal(
		fence.Get(),
		fenceValue
	);

	// GPUの処理完了を待つ
	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(
			fenceValue,
			fenceEvent
		);

		WaitForSingleObject(
			fenceEvent,
			INFINITE
		);
	}
}

void DirectXCommon::Finalize()
{
	if (fenceEvent != nullptr) {
		CloseHandle(fenceEvent);
		fenceEvent = nullptr;
	}
}
