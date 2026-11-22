#pragma once
#include "WinApp.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <array>
#include <string>
#include <chrono>
#include <thread>

// DirectXの基盤
class DirectXCommon
{
public:
	void Initialize(WinApp* winApp);
	void DeviceInitialize();
	void CommandInitialize();
	void SwapChainInitialize();
	void DescriptorHeapInitialize();
	void RenderTargetViewInitialize();
	void DepthbufferInitialize();
	void DepthStencilViewInitialize();
	void FenceInitialize();
	void ViewportInitialize();
	void ScissorRectInitialize();
	void DXCInitialize();
	void ImGuiInitialize();
	void InitializeFixFPS();
	void UpdateFixFPS();
	void Update();
	void PreDraw();
	void PostDraw();
	void Finalize();

private:

	WinApp* winApp = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	std::array< Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	uint32_t fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	static const uint32_t kRtvDescriptorCount = 2;
	uint32_t descriptorSizeRTV = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	uint32_t descriptorSizeSRV = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
	uint32_t descriptorSizeDSV = 0;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	Microsoft::WRL::ComPtr<ID3D12Resource> depthstencilResource;

	D3D12_VIEWPORT viewport{};

	D3D12_RECT scissorRect{};

	IDxcUtils* dxcUtils;
	IDxcCompiler3* dxcCompiler;
	IDxcIncludeHandler* includeHandler;

	int backBufferIndex = 0;

	D3D12_RESOURCE_BARRIER barrier{};

	std::chrono::steady_clock::time_point reference_;

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(int32_t width, int32_t height);



public:

	IDxcBlob* CompileShader(
		// compilerするShaderファイルへのパス
		const std::wstring& filePath,
		// compilerに使用するProfile
		const wchar_t* profile);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(
		size_t sizeInBytes);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(
		const DirectX::TexMetadata& metadata);

	DirectX::ScratchImage LoadTexture(const std::string& filePath);
	void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
		CreateDescriptorHeap(
			D3D12_DESCRIPTOR_HEAP_TYPE heapType,
			UINT numDescriptors,
			bool shaderVisible
		);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	ID3D12Device* GetDevice() { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList.Get(); }
	ID3D12CommandQueue* GetCommandQueue() { return commandQueue.Get(); }
	IDXGISwapChain4* GetSwapChain() { return swapChain.Get(); }
	ID3D12Fence* GetFence() { return fence.Get(); }
	IDxcUtils* GetDxcUtils() { return dxcUtils; }
	IDxcCompiler3* GetDxcCompiler() { return dxcCompiler; }
	IDxcIncludeHandler* GetIncludeHandler() { return includeHandler; }
	ID3D12DescriptorHeap* GetSRVDescriptorHeap() { return srvDescriptorHeap.Get(); };
	D3D12_RESOURCE_BARRIER* GetBarrier() { return &barrier; }

};


