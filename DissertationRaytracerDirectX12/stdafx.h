#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#include "ShaderBindingTableGenerator.h"
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <string>
#include <dxcapi.h>
#include <vector>
#include "TopLevelASGenerator.h"
#include <wrl/client.h>
#include "CameraController.h"

using Microsoft::WRL::ComPtr;



//#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
/*
template<typename T> _Post_equal_to_(pp) _Post_satisfies_(return == pp) void** IID_PPV_ARGS_Helper(T * *pp)
{
#pragma prefast(suppress:6269, "Tool issue with unused static_cast")
	static_cast<IUnknown*>(*pp);
	return reinterpret_cast<void*>(pp)
}
*/
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }




//Window Handle
HWND hwnd = NULL;


//Window name
LPCTSTR WindowName = L"Raytracer";

//Window title

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

ID3D12Resource* renderTargets[frameBufferCount];

ID3D12CommandAllocator* commandAllocator[frameBufferCount];

ID3D12GraphicsCommandList4* commandList;

ID3D12Fence* fence[frameBufferCount];

HANDLE fenceEvent;

UINT64 fenceValue[frameBufferCount];

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


struct AccelerationStructureBuffers
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pScratch;
	Microsoft::WRL::ComPtr<ID3D12Resource> pResult;
	Microsoft::WRL::ComPtr<ID3D12Resource> pInstanceDesc;
};


AccelerationStructureBuffers topLevelASBuffers;

std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> instances;




AccelerationStructureBuffers CreateBottomLevelAS(std::vector < std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers, std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers);


void CreateTopLevelAS(const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly);





void CreateAccelerationStructures();






LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool InitD3D();

void Update();

void UpdatePipeline();

void Render();

void createDepthBuffer();

void WaitForPreviousFrame();
void Cleanup();

void CheckRayTracingSupport();

ComPtr<ID3D12RootSignature> CreateRayGenSignature();

ComPtr<ID3D12RootSignature> CreateMissSignature();

ComPtr<ID3D12RootSignature> CreateHitSignature();
void CreateRaytracingPipeline();

void CreateRaytracingOutputBuffer();

void CreateShaderResourceheap();

void CreateShaderBindingTable();


void CreateCameraBuffer();

void UpdateCameraBuffer();

void CreateGlobalConstantBuffer();

void CreatePerInstanceBuffer();

ComPtr<ID3D12Resource> cameraBuffer;
ComPtr<ID3D12DescriptorHeap> constHeap;

ComPtr<ID3D12Resource> globalConstBuffer;

std::vector<ComPtr<ID3D12Resource>> perInstanceConstantBuffers;


uint32_t cameraBufferSize = 0;

ComPtr<ID3D12DescriptorHeap> dsvHeap;
ComPtr<ID3D12Resource> depthBuffer;


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

D3D12_INDEX_BUFFER_VIEW indexBufferView;

D3D12_VERTEX_BUFFER_VIEW planeBufferview;

void CreatePlaneVB();


void OnKeyUp(UINT8 key);


bool m_raster = true;


std::uint32_t game_time;

ComPtr<ID3D12Resource> perInstancePropertiesBuffer;

void CreatePerInstancePropertiesBuffer();

void UpdatePerInstanceProperties();

struct PerInstanceProperties {
	DirectX::XMMATRIX objectToWorld;
	DirectX::XMMATRIX objectToWorldNormal;
};
