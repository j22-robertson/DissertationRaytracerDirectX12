#pragma once
#include "stdafx.h"
#include "tiny_obj_loader.h"
#include "Imgui/imgui.h"
#include <DirectXTex.h>

//#define D3DCOMPILE_DEBUG
struct ApplicationSettings
{
	int height;
	int width;
};

struct PerInstanceProperties {
	DirectX::XMMATRIX objectToWorld;
	DirectX::XMMATRIX objectToWorldNormal;
};

struct AccelerationStructureBuffers
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pScratch;
	Microsoft::WRL::ComPtr<ID3D12Resource> pResult;
	Microsoft::WRL::ComPtr<ID3D12Resource> pInstanceDesc;
};


class App
{
public:
	virtual void OnKeyUp(UINT8 key) = 0;
};
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
auto app = 	reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	//App *app = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
return true;
	switch (msg)
	{
	case WM_CREATE:
		{
	//	DirectX::CreateTexture();
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		return 0;
		}
	case WM_KEYUP:
	{
		app->OnKeyUp(static_cast<UINT8>(wParam));
		//OnKeyUp(static_cast<UINT8>(wParam));


		return 0;
	}
	case WM_KEYDOWN:
	{
		if (wParam == VK_ESCAPE) {
			if (MessageBox(0, L"Are you sure you want to exit?", L"Confirm, do you want to exit program?", MB_YESNO | MB_ICONQUESTION) == IDYES)
				DestroyWindow(hWnd);
		}
		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	}
	return DefWindowProc(hWnd, msg, wParam, lParam);

}


struct Vertex {
	Vertex(float x, float y, float z, float r, float g, float b, float a) : pos(x, y, z), color(r, g, b, a) {}
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color;
};




class RenderApplication : App
{
	
public:
	~RenderApplication()
	{
	//	Cleanup();
	}
//** SHOULD NOT BE PUBLIC, CHANGE**//
	UINT teapotVertNumber;
	UINT teapotIndexNumber;

	std::string inputfile = "teapot.obj.txt";
	tinyobj::ObjReaderConfig reader_config;
	tinyobj::ObjReader reader;

	void OnKeyUp(UINT8 key) override;
//	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


int setup(HINSTANCE hInstance, int ShowWnd, int width, int height, bool fullscreen);
	void run();

private:
	/*
	 * Raytracing
	 */
	void CheckRaytracingSupport();
	void CreateRaytracingPipeline();

	
	void CreateRaytracingOutputBuffer();

	ComPtr<ID3D12Resource> env_texture;



	AccelerationStructureBuffers CreateBottomLevelAS(std::vector < std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers, std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers);
	void CreateTopLevelAS(const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly);

	void CreateAccelerationStructures();
	ComPtr<ID3D12RootSignature> CreateRayGenSignature();
	ComPtr<ID3D12RootSignature> CreateHitSignature();
	ComPtr<ID3D12RootSignature> CreateMissSignature();

	void CreateEnvmapResourceHeap();

	void CreateShaderResourceheap();
	void CreateShaderBindingTable();
	

	DirectX::XMVECTOR bgcolour;
	/*
	 *
	 */
	void createDepthBuffer();
	void CreatePerInstanceBuffer();
	void createBackgroundBuffer();
	void UpdateBackgroundBuffer();

	void LoadEnvironmentMap(const wchar_t* filename);
	DirectX::TexMetadata data;
	std::unique_ptr<DirectX::ScratchImage> image;

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	void LoadTextureFromFile(const wchar_t* filename);



	void CreatePerInstancePropertiesBuffer();
	void UpdatePerInstancePropertiesBuffer();



	void CreateCameraBuffer();
	void UpdateCameraBuffer();


	
	void CreatePlaneVB(); 

	/*
	 *
	 */

	bool InitD3D();
	bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool fullscreen);
	void mainloop();
	void UpdatePipeline();

	void WaitForPreviousFrame();

	void Cleanup();

	void update();
	void Render();


	//void OnKeyUp(UINT8 key);




private:
	HWND hwnd = NULL;


	//Window name
	LPCTSTR WindowName = L"Raytracer";

	//Window title
	ComPtr<ID3D12Debug> debugController;

	LPCTSTR WindowTitle = L"Raytracer";

	int Width = 1920;
	int Height = 1080;


	bool FullScreen = false;

	bool Running = true;

	bool tempBool = true;

	const int frameBufferCount = 3; // buffer num, 2 = double buffering 3 = triple


	ID3D12Device5* device;

	IDXGISwapChain3* swapChain;

	ID3D12CommandQueue* commandQueue;

	ID3D12DescriptorHeap* rtvDescriptorHeap; // Holds resources e.g. render targets

	ID3D12Resource* renderTargets[/*Change back to fbuffercount*/3];

	ID3D12CommandAllocator* commandAllocator[/*change back to fbuffercount*/3];

	ID3D12GraphicsCommandList4* commandList;

	ID3D12Fence* fence[/*Change back to fbuffercount*/ 3];

	HANDLE fenceEvent;

	UINT64 fenceValue[/*Change back to fbuffercount*/ 3];

	int frameIndex;

	int rtvDescriptorSize;

	ID3D12PipelineState* pipelineStateObject;

	ID3D12RootSignature* rootSignature;

	D3D12_VIEWPORT viewport;

	D3D12_RECT scissorRect;

	ID3D12Resource* vertexBuffer;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	//RAYTRACING STUFF

	ID3D12Resource* bottomLevelAS;

	nv_helpers_dx12::TopLevelASGenerator topLevelASGenerator;


	ComPtr<ID3D12Resource> textureUploadHeap;


	AccelerationStructureBuffers topLevelASBuffers;

	std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> instances;

	ComPtr<ID3D12Resource> cameraBuffer;
	ComPtr<ID3D12DescriptorHeap> constHeap;

	ComPtr<ID3D12Resource> globalConstBuffer;

	std::vector<ComPtr<ID3D12Resource>> perInstanceConstantBuffers;


	uint32_t cameraBufferSize = 0;

	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12Resource> depthBuffer;

	ComPtr<ID3D12DescriptorHeap> envmapHeap;

	nv_helpers_dx12::ShaderBindingTableGenerator sbtHelper;
	ComPtr<ID3D12Resource> sbtStorage;
	ComPtr<ID3D12Resource> outputResource;
	ComPtr<ID3D12DescriptorHeap> srvUAVHeap;

	CameraController cameraController;

	ComPtr<IDxcBlob> rayGenLibrary;
	ComPtr<IDxcBlob> hitLibrary;
	ComPtr<IDxcBlob> missLibrary;
	ComPtr<IDxcBlob> shadowLibrary;


	ComPtr<ID3D12RootSignature> rayGenSignature;
	ComPtr<ID3D12RootSignature> hitSignature;
	ComPtr<ID3D12RootSignature> missSignature;
	ComPtr<ID3D12RootSignature> shadowHitSignature;


	ComPtr<ID3D12StateObject> rtStateObject;

	ComPtr<ID3D12StateObjectProperties> rtStateObjectProps;


	ComPtr<ID3D12Resource> planeBuffer;

	ComPtr<ID3D12Resource> indexBuffer;
	ComPtr<ID3D12Resource> backgroundColor;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	D3D12_VERTEX_BUFFER_VIEW planeBufferview;
	bool m_raster = true;



	//TODO: Remove temp rotspeed variable
	float rotspeed = 50.0;
	std::uint32_t game_time;

	ComPtr<ID3D12Resource> perInstancePropertiesBuffer;
	DirectX::XMVECTOR bgcol;
};

