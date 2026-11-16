#include <string>
#include <format>
#include <strsafe.h>
#include <fstream>
#include <sstream>
#include <xaudio2.h>
#include "DebugCamera.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "Input.h"
#include "DirectXCommon.h"
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"xaudio2.lib")
// ファイルやディレクトリに関する操作を行うライブラリ
#include <filesystem>
// 時間を扱うライブラリ
#include <chrono>
// ファイルに書いたり読んだりするライブラリ

// Debug用のあれやこれやを使えるようにする
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")

struct Vector4
{
	float x, y, z, w;
};

struct Transforms {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct Matrix3x3
{
	float m[3][3];
};

struct VertexData
{
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material
{
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};


struct MaterialData
{
	std::string textureFilePath;
};
struct ModelData
{
	std::vector<VertexData> vertices;
	MaterialData material;
};
struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight
{
	Vector4 color; //!< ライトの色
	Vector3 direction; //!< ライトの向き
	float intensity; //!< 輝度
};


struct D3DResourceLeakChecker
{
	~D3DResourceLeakChecker()
	{
		// リソースチェック
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

// チャンクヘッダ
struct ChunkHeader
{
	char id[4]; // チャンク毎のID
	int32_t size; // チャンクサイズ
};

// RIFFヘッダチャンク
struct RiffHeader
{
	ChunkHeader chunk;  // "RIFF"
	char type[4]; // "WAVE"
};

// FMTチャンク
struct FormatChunk
{
	ChunkHeader chunk; // "fmt"
	WAVEFORMATEX fmt;   // 波系フォーマット

};

// 音声データ
struct SoundData
{
	// 波系フォーマット
	WAVEFORMATEX wfex;

	// バッファの先頭アドレス
	BYTE* pBuffer;

	// バッファのサイズ
	unsigned int bufferSize;

};

float materialColor[4] =
{
	1.0f,
	1.0f,
	1.0f,
	1.0f
};


Matrix4x4 MakeScaleMatrix(const Vector3& scale)
{
	Matrix4x4 result =
	{
		scale.x, 0.0f,    0.0f,    0.0f,
		0.0f,    scale.y, 0.0f,    0.0f,
		0.0f,    0.0f,    scale.z, 0.0f,
		0.0f,    0.0f,    0.0f,    1.0f
	};

	return result;
}

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{



	// 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリいかに出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath,
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE |
		FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processId（このexeとId）とクラッシュ（例外）の発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	//設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;

	// Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

	// 他に関連づけられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
	return EXCEPTION_EXECUTE_HANDLER;
}

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam) {
#ifdef USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
	// メッセージに応じてゲーム固有の処理を行う
#endif // USE_IMGUI
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



// ログ
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}
void Log(std::ostream& os, const std::string& message)
{
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}


void ALog(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}

void ALog(const std::wstring& message)
{
	ALog(ConvertString(message));
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {

	// 1.中で必要になるファイルの宣言
	MaterialData materialData; // 構築するMaterialData
	std::string line; // ファイルから読んだ1行を格納するもの

	// 2.ファイルを開く
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める

	// 3.実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;

			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	// 4.MaterialData を返す
	return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {

	// 1. 中で必要となる変数の宣言
	ModelData modelData; // 構築するModelData

	std::vector<Vector4> positions; // 位置
	std::vector<Vector3> normals; // 法線
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line; // ファイルから読んだ1行を格納するもの

	// 2. ファイルを開く
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める

	// 3. 実際にファイルを読み、ModelDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む

		// identifierに応じた処理 v:位置
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		}                   // vt:テクスチャ座標
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
		}                   // vn:法線
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}                   // f:面
		else if (identifier == "f") {

			VertexData triangle[3];

			// 面は三角限定。そのほかは未対応
			for (int32_t faceVertex = 0; faceVertex < 3;++faceVertex) {

				//．．．ここは今までと同じ．．．
				std::string vertexDefinition;
				s >> vertexDefinition;

				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3;++element) {
					std::string index;
					std::getline(v, index, '/'); // /区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}

				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				position.x *= -1;
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				normal.x *= -1;
				texcoord.y = 1.0f - texcoord.y;
				VertexData vertex = { position,texcoord,normal };

				triangle[faceVertex] = { position,texcoord,normal };
			}
			// 頂点を逆順で登録することで、周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;

			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	// 4. ModelDataを返す
	return modelData;
}


SoundData SoundLoadWave(const char* filename) {

	// 1. ファイルオープン
	// ファイル入力ストリームのインスタンス
	std::ifstream file;

	// .wavファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);

	// ファイルオープン失敗を検出する
	assert(file.is_open());

	// 2. .wavデータ読み込み
	// RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));

	// ファイルがRIFFかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	// Formatチャンクの読み込み
	FormatChunk format = {};

	// チャンクヘッダーの確認
	file.read((char*)&format, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	// チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	// Dataチャンクの読み込み
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));

	// JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0) {

		// 読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);

		// 再読み込み
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	// Dataチャンクのデータ部（波形データ)の読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	// 3. ファイルクローズ
	// Waveファイルを閉じる
	file.close();

	// 4. 読み込んだ音声データをreturn
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

// 音声データの解散
void SoundUnload(SoundData* soundData) {

	// バッファののメモリを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}


void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData) {

	HRESULT hr;

	// 波形フォーマットを基にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	hr = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(hr));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	// 波形データの再生
	hr = pSourceVoice->SubmitSourceBuffer(&buf);
	hr = pSourceVoice->Start();
}


IDxcBlob* CompileShader(

	// compilerするShaderファイルへのパス
	const std::wstring& filePath,

	// compilerに使用するProfile
	const wchar_t* profile,

	// 初期化で生成したものを3つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler)
{
	// ここの中身をこの後書いていく

	// 1. hlslファイルを読む
	// これからシェーダーをコンパイルする旨をログに出す
	ALog(std::format(L"begin CompileShader, path:{}, profile:{}\n", filePath, profile));

	// hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

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
		ALog(shaderError->GetStringPointer());

		// 警告・エラーダメ絶対
		assert(false);
	}

	// 4. compile結果をを受け取って返す
	// コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// 成功したログを出す
	ALog(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));


	// 実行用のバイナリを返却
	return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t sizeInBytes) {

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

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {

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

DirectX::ScratchImage LoadTexture(const std::string& filePath) {

	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの制作
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミニマップ付きのデータを返す
	return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metadata) {

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

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height)
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

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;

}



void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages)
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






//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{


	//assert(false && &"assertのテストだよ");

	D3DResourceLeakChecker leakCheck;
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));
	SetUnhandledExceptionFilter(ExportDump);

	WinApp* winApp = nullptr;

	winApp = new WinApp();
	winApp->Initialize();

	DirectXCommon* dxCommon = nullptr;
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	/////////////////////////////////////////////////////////////↑ここまでWindow処理


	// 文字列を格納
	std::string str0 = { "STRING!!!" };

	// 整数を文字列にする
	std::string str1{ std::to_string(10) };


	// ログのディレクトリを用意
	std::filesystem::create_directory("logs");

	// 現在時刻を取得（UTC時刻）
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
	std::chrono::time_point < std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	// 日本時間（PCの設定時間）に変換
	std::chrono::zoned_time localeTime{ std::chrono::current_zone(),nowSeconds };

	// formatを使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localeTime);

	// 時刻を使ってファイル名を決定
	std::string logFilePath = std::string("logs/") + dateString + ".log";

	// ファイルを作って書き込み準備
	std::ofstream logStream(logFilePath);


	//////////////////////////////////////////////////////////↓ここからDirectX12

	//dxcommon->DeviceInitialize();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	//dxCommon->CommandListInitialize();

	///////////////////////////////////////////////////////////////////////////////////↓スワップチェーン

	//dxCommon->SwapChainInitialize();	


	// dxCommon->RenderTargetViewInitialize();

	// 描画先のRTVを設定する
	//commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], FALSE, nullptr);
	//// 指定した色で画面全体をクリアする
	//float clearColor[] = { 0.1f,0.25f,0.5f,1.0f }; // 青っぽい色。RGBAの順
	//commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeaps[] = { srvDescriptorHeap };
	//commandList->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());


	// 初期値0でFenceを作る
// dxCommon->FenceInitialize();


// dxcCompilerを初期化
	// dxCommon->DxcCompilerInitialize();

	// Input呼び出し

	Input* input = nullptr;
	input = new Input();
	input->Initialize(winApp);



	///////////////////////////////////////////////////////////////////////////////////

	const uint32_t descriptorSizeSRV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(dxCommon->GetDevice(), metadata);
	UploadTextureData(textureResource, mipImages);

	// モデル読み込み
	ModelData modelData = LoadObjFile("resources", "plane.obj");

	// 2枚目のTextureを読んで転送する
	DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(dxCommon->GetDevice(), metadata2);
	UploadTextureData(textureResource2, mipImages2);

	// metaDataを基にSRVを設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// metaDataを基にSRVを設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = dxCommon->GetSRVCPUDescriptorHandle(1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = dxCommon->GetSRVGPUDescriptorHandle(1);

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = dxCommon->GetSRVCPUDescriptorHandle(2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = dxCommon->GetSRVGPUDescriptorHandle(2);

	// 先頭はImGuiが使っているのでその次を使う
	textureSrvHandleCPU.ptr += dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU.ptr += dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// SRVの生成
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);

	dxCommon->GetDevice()->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

	////////////////////////////////////////////////////////////////////////////////////////////

	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// RootParameter作成。複数設定できるので配列。今回は結果1つだけなので長さ1の配列
	// RootPrameterへ追加。PixelShaderのMaterialとVertexShaderのTransform
	D3D12_ROOT_PARAMETER rootParameters[4] = {};

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRange[0].NumDescriptors = 1; // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Oddsetを自動計算

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VaertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	descriptionRootSignature.pParameters = rootParameters; // ルートパラメーター配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中身の配列指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号1を使う


	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR; // バイリニフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピーター
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0; // レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);





	// シリアライズしてバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		ALog(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	// バイナリをもとに生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));


	// Inputlayout
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[3] = {};
	inputElementDesc[0].SemanticName = "POSITION";
	inputElementDesc[0].SemanticIndex = 0;
	inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDesc[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDesc[1].SemanticName = "TEXCOORD";
	inputElementDesc[1].SemanticIndex = 0;
	inputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDesc[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDesc[2].SemanticName = "NORMAL";
	inputElementDesc[2].SemanticIndex = 0;
	inputElementDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDesc[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDesc;
	inputLayoutDesc.NumElements = _countof(inputElementDesc);

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};

	// すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterrizeStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 裏面（時計回り）を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイルする
	IDxcBlob* vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl",
		L"vs_6_0", dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler());

	IDxcBlob* pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl",
		L"ps_6_0", dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler());
	assert(pixelShaderBlob != nullptr);
	assert(vertexShaderBlob);
	assert(pixelShaderBlob);

	assert(vertexShaderBlob->GetBufferPointer());
	assert(pixelShaderBlob->GetBufferPointer());

	assert(vertexShaderBlob->GetBufferSize() > 0);
	assert(pixelShaderBlob->GetBufferSize() > 0);
	// PSO生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();// RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc; // InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };// VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };// Pixelshader
	graphicsPipelineStateDesc.BlendState = blendDesc; // BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc; // RasterizerState

	//書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//どのように画面に色を打ち込むかの設定(気にしなくて良い)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//実際に生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));



	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(uint32_t) * 6);

	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();

	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;

	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataSprite = nullptr;

	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));

	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;


	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLight = CreateBufferResource(dxCommon->GetDevice(), sizeof(DirectionalLight));

	DirectionalLight* directionalLightData = nullptr;

	directionalLight->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;


	// Sprite用の頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(VertexData) * 6);

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};

	// リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();

	// 1頂点あたりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	// 使用するリソースのサイズは6つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;

	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	// 1枚目の三角形
	vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };   // 左下
	vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };     // 左上
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f }; // 右下
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };

	// 2枚目の三角形
	vertexDataSprite[3].position = { 0.0f,0.0f,0.0f,1.0f };     // 左上
	vertexDataSprite[3].texcoord = { 0.0f,0.0f };
	vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f };   // 右上
	vertexDataSprite[4].texcoord = { 1.0f,0.0f };
	vertexDataSprite[4].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f }; // 右下
	vertexDataSprite[5].texcoord = { 1.0f,1.0f };
	vertexDataSprite[5].normal = { 0.0f,0.0f,-1.0f };


	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMaterialResourceObj = CreateBufferResource(dxCommon->GetDevice(), sizeof(TransformationMatrix));

	// データを書き込む
	TransformationMatrix* transformationMaterialDataObj = nullptr;

	// 書き込むためのアドレスを取得
	transformationMaterialResourceObj->Map(0, nullptr, reinterpret_cast<void**>(&transformationMaterialDataObj));

	// 単位行列を書き込んでおく
	transformationMaterialDataObj->World = MakeIdentity4x4();
	transformationMaterialDataObj->WVP = MakeIdentity4x4();




	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(TransformationMatrix));

	// データを書き込む

	TransformationMatrix* transformationMatrixDataSprite = nullptr;

	// 書き込むためのアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));


	// 単位行列を書き込んでおく
	transformationMatrixDataSprite->World = MakeIdentity4x4();
	transformationMatrixDataSprite->WVP = MakeIdentity4x4();

	// 実際に頂点リソースを作る
	const uint32_t kSubdivision = 32; // 分割数

	const uint32_t vertexCount = kSubdivision * kSubdivision * 6;

	//ID3D12Resource* vertexResource =
	//	CreateBufferResource(device, sizeof(VertexData) * vertexCount);
	//assert(SUCCEEDED(hr));


	// 頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = CreateBufferResource(dxCommon->GetDevice(), sizeof(VertexData) * modelData.vertices.size());

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferview{};

	// リソースの先頭アドレスから使う
	vertexBufferview.BufferLocation = vertexResource->GetGPUVirtualAddress();

	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferview.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());

	// 1頂点あたりのサイズ
	vertexBufferview.StrideInBytes = sizeof(VertexData);


	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());


	// マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceObj = CreateBufferResource(dxCommon->GetDevice(), sizeof(Material));

	// マテリアルにデータを書き込む
	Material* materialDataObj = nullptr;

	// 書き込むためのアドレスを取得
	materialResourceObj->Map(0, nullptr, reinterpret_cast<void**>(&materialDataObj));

	// 今回は赤を書き込んでみる
	materialDataObj->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialDataObj->enableLighting = 1;
	materialDataObj->uvTransform = MakeIdentity4x4();

	// マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(Material));

	// マテリアルにデータを書き込む
	Material* materialDataSprite = nullptr;

	// 書き込むためのアドレスを取得
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));

	// 今回は赤を書き込んでみる
	materialDataSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialDataSprite->enableLighting = 0;
	materialDataSprite->uvTransform = MakeIdentity4x4();

	// DepthStecilTextureをウィンドウのサイズで作成

	// dxCommon->DepthbufferInitialize();
	// dxCommon->DepthStencilViewInitialize();

	// DepthstencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// Depthの機能を有効にする
	depthStencilDesc.DepthEnable = true;

	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	// 比較関数はLessWqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(
		&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));

	assert(SUCCEEDED(hr));
	
	
	assert(vertexBufferview.BufferLocation != 0);
	assert(vertexBufferview.SizeInBytes != 0);
	assert(vertexBufferview.StrideInBytes != 0); 

	// 誰も捕捉しなかった場合に(Unhandled)、補足する関数を登録
	// main関数始まってすぐに登録すると良い

	MSG msg{};

	Transforms transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Transforms cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-5.0f} };

	// Sprite用
	Transforms transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Transforms uvTransformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };


	DebugCamera* debugCamera = nullptr;

	debugCamera = new DebugCamera();
	debugCamera->Initialize();


	bool useMonsterBall = false;



	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;

	// XAudioエンジンのインスタンスを生成
	hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

	// マスターボイス生成
	hr = xAudio2->CreateMasteringVoice(&masterVoice);

	// 音声読み込み
	SoundData soundData1 = SoundLoadWave("Resources/Alarm01.wav");

	// 音声再生
	SoundPlayWave(xAudio2.Get(), soundData1);

	//ウィンドウボタンの×ボタンが押されるまでループ
	while (true) {

		// Windowsのメッセージ処理
		if (winApp->ProcessMessage()) {

			// ゲームループを抜ける
			break;
		}
		else {

			// キーボード情報の取得開始
			input->Update();

			// 数字の0キーが押されていたら
			if (input->TriggerKey(DIK_0))
			{
				OutputDebugStringA("Hit 0\n"); // 出力ウィンドウに「Hit 0」と表示
			}
			debugCamera->Update(/*{io.MouseDelta.x, io.MouseDelta.y}*/);

			//ゲームの処理
			//transform.rotate.y += 0.03f;
			Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 viewMatrix = debugCamera->GetViewMatrix();
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldviewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

			transformationMaterialDataObj->WVP = worldviewProjectionMatrix;
			transformationMaterialDataObj->World = worldMatrix;
			/*transformationData->WVP = worldviewProjectionMatrix;
			transformationData->World = worldMatrix;*/


			// Sprite用
			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
			Matrix4x4 worldviewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
			transformationMatrixDataSprite->WVP = worldviewProjectionMatrixSprite;
			transformationMatrixDataSprite->World = MakeIdentity4x4();





			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_IMGUI
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();

			ImGui::NewFrame();
			// 開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
			ImGui::ShowDemoWindow();
			ImGui::Begin("Debug");



			if (ImGui::TreeNode("Cameras")) {
				ImGui::DragFloat3("CameraTranslate", &cameraTransform.translate.x, 0.01f, -20.0f);
				ImGui::SliderAngle("CameraRotateX", &cameraTransform.rotate.x);
				ImGui::SliderAngle("CameraRotateY", &cameraTransform.rotate.y);
				ImGui::SliderAngle("CameraRotateZ", &cameraTransform.rotate.z);
				ImGui::DragFloat3("CameraScale", &cameraTransform.scale.x, 0.1f, 20.0f);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Obj")) {

				ImGui::SliderAngle("objRotateX", &transform.rotate.x);
				ImGui::SliderAngle("objRotateY", &transform.rotate.y);
				ImGui::SliderAngle("objRotateZ", &transform.rotate.z);


				ImGui::TreePop();
			}



			if (ImGui::TreeNode("Ligthing")) {

				ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.01f, 10.0f);
				ImGui::DragFloat("LightIntensity", &directionalLightData->intensity, 0.01f, 10.0f);
				ImGui::DragFloat4("LightColor", &directionalLightData->color.x);

				ImGui::TreePop();
			}




			ImGui::ColorEdit4("Color", materialColor);
			//ImGui::Checkbox("useMonsterBall", &useMonsterBall);

			if (ImGui::TreeNode("Sprite")) {

				ImGui::DragFloat2("spriteTranslate", &transformSprite.translate.x, 0.1f, -10.0f, 1280.0f);
				ImGui::DragFloat2("spriteScale", &transformSprite.scale.x, 0.1f, -10.0f, 1280.0f);
				ImGui::SliderAngle("UVTRotate", &transformSprite.rotate.z);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("UVTransform")) {

				ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
				ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
				ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

				ImGui::TreePop();
			}





			ImGui::End();
			ImGui::Render();

#endif // USE_IMGUI

			dxCommon->PreDraw();

			Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
			uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
			uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
			materialDataObj->color = {
				materialColor[0],
				materialColor[1],
				materialColor[2],
				materialColor[3]
			};

			materialDataObj->enableLighting = 1;
			materialDataSprite->uvTransform = uvTransformMatrix;


			dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
			dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
			dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferview);
			//dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceObj->GetGPUVirtualAddress());
			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLight->GetGPUVirtualAddress());




			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heaps[] = {
			 dxCommon->GetSRVDescriptorHeap()
			};
			dxCommon->GetCommandList()->SetDescriptorHeaps(1, heaps->GetAddressOf());
			// 実際のcommandListのImGuiの描画コマンドを積む
			dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);



			//--------------------------------------
			// 三角形描画
			//--------------------------------------


			dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
			dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
			dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferview);
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMaterialResourceObj->GetGPUVirtualAddress());

			materialDataObj->enableLighting = 1;
			dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

			// スプライト用
			dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);
			//dxCommon->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);

			//dxCommon->GetCommandList()->DrawInstanced(6, 1, 0, 0);


			//--------------------------------------
			// ImGui描画
			//--------------------------------------
#ifdef USE_IMGUI
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());
#endif // USE_IMGUI

			dxCommon->PostDraw();

		}

	}
#ifdef USE_IMGUI
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif




	// 解放処理
	delete input;
	delete winApp;
	winApp = nullptr;

	//xAudio2解放
	xAudio2.Reset();

	// 音声データ解放
	SoundUnload(&soundData1);
	winApp->Finalize();

	return 0;
}
