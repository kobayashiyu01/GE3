#include "DirectXCommon.h"
#include "Logger.h"
#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib,"dxgi.lib")


using namespace Microsoft::WRL;

IDxcBlob* DirectXCommon::CompileShader(
	// compilerするShaderファイルへのパス
	const std::wstring& filePath,
	// compilerに使用するProfile
	const wchar_t* profile
)
{
	// ここの中身をこの後書いていく

	// 1. hlslファイルを読む
	// これからシェーダーをコンパイルする旨をログに出す
	Logger::ALog(std::format(L"begin CompileShader, path:{}, profile:{}\n", filePath, profile));

	// hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr;
	hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

	// 読めなかったら止める
	assert(SUCCEEDED(hr));

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF8の文字コードであることを通知

	// 2. compileする
	LPCWSTR arguments[] = {
		filePath.c_str(),        // コンパイル対象のhlslファイル名
		L"-E",L"main",           // エントリーポイントの指定。基本的にmain以外にはしない
		L"-T",profile,           // ShaderProfileの設定
		L"-Zi",L"-Qembed_debug", // デバッグ用の情報を埋め込む
		L"-Od",                  // 最適解を外してる
		L"-Zpr",                 // メモリレイアウトの行優先
	};

	//  実際にShaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,         // 読み込んだファイル
		arguments,                   // コンパイルオプション
		_countof(arguments),         // コンパイルオプションの数
		includeHandler,              // includeが含まれたもろもろ
		IID_PPV_ARGS(&shaderResult)  // コンパイル結果
	);

	// コンパイルエラーではなくdxcが起動できない時などの致命的な状況
	assert(SUCCEEDED(hr));

	// 3. 警告・エラーが出ていないか確認する
	// 警告・エラーが出たらログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Logger::ALog(shaderError->GetStringPointer());

		// 警告・エラーダメ絶対
		assert(false);
	}

	// 4. compile結果をを受け取って返す
	// コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// 成功したログを出す
	Logger::ALog(stringUtility::ConverString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));


	// 実行用のバイナリを返却
	return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes) {

	// 頂点リソース用のバイナリヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeap 5

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};

	//バッファリソース。テクスチャの場合はまた別の設定をする
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;//リソースのサイズ。今回はVector4を3頂点分

	// バッファの場合はこれらは1にする決まり
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;

	// バッファの場合はこれにする決まり
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource));

	return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(
	const DirectX::TexMetadata& metadata) {

	// 1. metadataをもとにResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width); // Textureの幅
	resourceDesc.Height = UINT(metadata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行き or 配列Textureの配列数
	resourceDesc.Format = metadata.format; // Textureのformat
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは2次元

	// 2. 利用するHeapの設定
	// 利用するHeapの設定。非常に特殊な運用。02_04exで一般的なケース版がある
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM; // 細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置

	// 3. Resourceを生成する
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし。
		&resourceDesc, // Resourceの設定
		D3D12_RESOURCE_STATE_GENERIC_READ, // 初回のResourceState。Textureは基本読むだけ
		nullptr,
		IID_PPV_ARGS(&resource)); // 作成するresourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}


DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string& filePath) {

	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = stringUtility::ConverString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの制作
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミニマップ付きのデータを返す
	return mipImages;
}


void DirectXCommon::UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages)
{
	// Meta情報を取得
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	// 全MipMapについて
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {

		// MipMapLevelを指定して各Imageを取得
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);

		// Textureに転送
		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel),
			nullptr,              // 全領域へコピー
			img->pixels,          // 元データアドレス
			UINT(img->rowPitch),  // 1ラインサイズ
			UINT(img->slicePitch) // 1枚サイズ
		);
		assert(SUCCEEDED(hr));
	}

}

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
	includeHandler = nullptr;


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
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
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
