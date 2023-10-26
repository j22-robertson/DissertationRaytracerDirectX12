#include "stdafx.h"
#include "DXRHelper.h"
#include "BottomLevelASGenerator.h"
#include "RaytracingPipelineGenerator.h"
#include "RootSignatureGenerator.h"


struct Vertex {
	Vertex(float x, float y, float z, float r, float g, float b, float a) : pos(x, y, z), color(r, g, b, a) {}
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color;
};




bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool fullscreen)
{
	if (fullscreen)
	{
		HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

		MONITORINFO mi = { sizeof(mi) };

		GetMonitorInfo(hmon, &mi);

		width = mi.rcMonitor.right - mi.rcMonitor.left;

		height = mi.rcMonitor.bottom - mi.rcMonitor.top;

	}


	//Window structure setup
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WindowName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);





	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Error registering class", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}


	hwnd = CreateWindowEx(NULL,
		WindowName,
		WindowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hwnd)
	{
		MessageBox(NULL, L"Error creating window", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (fullscreen)
	{
		SetWindowLong(hwnd, GWL_STYLE, 0);
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	return true;


}



void mainloop()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));


	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			Render();
			//Update loop goes here
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYUP:
		{
			OnKeyUp(static_cast<UINT8>(wParam));

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	if (!InitializeWindow(hInstance, nCmdShow, Width, Height, FullScreen))
	{
		MessageBox(0, L"Window Initializaiton - Failed", L"Error", MB_OK);
		return 0;
	}
	if (!InitD3D())
	{
		MessageBox(0, L"Failed to initialize DX12",L"Error", MB_OK);

		Cleanup();
		return 1;
	}
	mainloop();

	WaitForPreviousFrame();
	CloseHandle(fenceEvent);

	Cleanup();


	return 0;
}
bool InitD3D()
{
	HRESULT hr;

	IDXGIFactory4* dxgiFactory;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	if (FAILED(hr))
	{
		return false;
	}

	IDXGIAdapter1* adapter; // this is the GPU device

	int adapterIndex = 0;

	bool adapterFound = false;

	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr);

		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}
	if (!adapterFound)
	{
		return false;
	}

	hr = D3D12CreateDevice(
		adapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&device)
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"Failed to create Device", L"Error", MB_OK);
		return false;
	}


	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};

	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;


	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));

	if (FAILED(hr))
	{
		MessageBox(0, L"Failed to create Command Queue", L"Error", MB_OK);
		return false;
	}

	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = Width;
	backBufferDesc.Height = Height;
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


	DXGI_SAMPLE_DESC sampleDesc{};
	sampleDesc.Count = 1;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = { };

	swapChainDesc.BufferCount = frameBufferCount;
	swapChainDesc.BufferDesc = backBufferDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.Windowed = !FullScreen;

	IDXGISwapChain* tempSwapChain;

	dxgiFactory->CreateSwapChain(
		commandQueue,
		&swapChainDesc,
		&tempSwapChain
	);



	swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

	rtvHeapDesc.NumDescriptors = frameBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));

	if (FAILED(hr))
	{
		MessageBox(0, L"Failed to create Descriptor heap", L"Error", MB_OK);
		return false;
	}

	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));

		if (FAILED(hr))
		{
			return false;
		}

		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

		rtvHandle.Offset(1, rtvDescriptorSize);
	}


	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
		if (FAILED(hr))
		{
			return false;
		}
	}


	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex], NULL, IID_PPV_ARGS(&commandList));

	if (FAILED(hr))
	{
		MessageBox(0, L"Failed to create Command List", L"Error", MB_OK);
		return false;
	}

	//commandList->Close();

	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));

		if (FAILED(hr))
		{
			return false;
		}
		fenceValue[i] = 0;

	}

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (fenceEvent == nullptr)
	{
		return false;
	}


	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;

	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);

	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	if (FAILED(hr))
	{
		return false;
	}


	ID3DBlob* vertexShader;

	ID3DBlob* errorBuff;

	hr = D3DCompileFromFile(L"VertexShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vertexShader,
		&errorBuff);

	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};

	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();

	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	ID3DBlob* pixelShader;
	
	hr = D3DCompileFromFile(L"PixelShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pixelShader,
		&errorBuff);

	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};

	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();



	/// INPUT LAYOUT

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};


	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// Pipeline state object

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = rootSignature;
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 1;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));

	if (FAILED(hr))
	{
		return false;
	}




	//// TEMP TRIANGLE SETUP

	//triangle
	Vertex vList[] = {
	{ 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
	{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	};


	int vBufferSize = sizeof(vList);

	//FIX LVLALUE
	auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resource_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);


	device->CreateCommittedResource(
		&heap_props,
		D3D12_HEAP_FLAG_NONE,
		&resource_buffer_desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer));

	vertexBuffer->SetName(L"Vertex Buffer Resource Heap");




	//CREATE UPLOAD HEAP
	// USED TO UPLOAD DATA TO GPU, CPU WRITES GPU READS

	ID3D12Resource* vBufferUploadHeap = {};
	//FIX LVALUE
	auto vbuffer_heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	auto vbuffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);

	device->CreateCommittedResource(
		&vbuffer_heap_props,
		D3D12_HEAP_FLAG_NONE,
		&vbuffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap)
	);

	vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	D3D12_SUBRESOURCE_DATA vertexData = {};

	vertexData.pData = reinterpret_cast<BYTE*>(vList);
	vertexData.RowPitch = vBufferSize;
	vertexData.SlicePitch = vBufferSize;



	//Upload heap from CPU to GPU

	UpdateSubresources(commandList,
		vertexBuffer,
		vBufferUploadHeap,
		0,
		0,
		1,
		&vertexData);

	//FIX LVALUE
	auto rbtemp = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	commandList->ResourceBarrier(1, &rbtemp);

	CheckRayTracingSupport();

	CreateAccelerationStructures();


	CreateRaytracingPipeline();



	// Allocate memory buffer storing the RayTracing output. Has same DIMENSIONS as the target image
	CreateRaytracingOutputBuffer();

	// Create the buffer containing the results of RayTracing (always output in a UAV), and create the heap referencing the resources used by the RayTracing e.g. acceleration structures
	CreateShaderResourceheap();

	CreateShaderBindingTable();
	commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//Increment fence otheriwse buffer may not be uploaded by draw time
	fenceValue[frameIndex]++;
	
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);

	if (FAILED(hr))
	{
		Running = false;
	}

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vBufferSize;

	// Fill out the Viewport
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = Width;
	scissorRect.bottom = Height;




	return true;

}

void UpdatePipeline() {
	HRESULT hr;

	WaitForPreviousFrame();

	hr = commandAllocator[frameIndex]->Reset();

	if (FAILED(hr))
	{
		Running = false;
	}


	hr = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);

	if (FAILED(hr))
	{
		Running = false;
	}

		commandList->SetGraphicsRootSignature(rootSignature);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

	///TODO: fix l-value for rb and rb2

	auto rb = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &rb);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	if (m_raster)
	{
		const float clearColor[] = { 0.0f,0.2f,0.4f,1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->DrawInstanced(3, 1, 0, 0);

	}
	else {
	//	const float clearColor[] = { 0.0f,0.2f,0.4f,1.0f };
	//	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		std::vector<ID3D12DescriptorHeap*> heaps = { srvUAVHeap.Get() };
		commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		commandList->ResourceBarrier(1, &transition);

		D3D12_DISPATCH_RAYS_DESC desc = {};

		uint32_t rayGenerationSectionSizeInBytes = sbtHelper.GetRayGenSectionSize();

		desc.RayGenerationShaderRecord.StartAddress = sbtStorage->GetGPUVirtualAddress();
		desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

		uint32_t missSectionSizeInBytes = sbtHelper.GetMissSectionSize();

		desc.MissShaderTable.StartAddress = sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
		desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
		desc.MissShaderTable.StrideInBytes = sbtHelper.GetMissEntrySize();

		uint32_t hitGroupSectionSize = sbtHelper.GetHitGroupSectionSize();
		desc.HitGroupTable.StartAddress = sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes + missSectionSizeInBytes;
		desc.HitGroupTable.SizeInBytes = hitGroupSectionSize;
		desc.HitGroupTable.StrideInBytes = sbtHelper.GetHitGroupEntrySize();
		desc.Width = Width;
		desc.Height = Height;
		desc.Depth = 1;

		commandList->SetPipelineState1(rtStateObject.Get());
		commandList->DispatchRays(&desc);

		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

		
		commandList->ResourceBarrier(1, &transition);

		transition = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);


		commandList->ResourceBarrier(1, &transition);
		commandList->CopyResource(renderTargets[frameIndex], outputResource.Get());

		transition = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);


		commandList->ResourceBarrier(1, &transition);

	}



	auto rb2 = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &rb2);

	hr = commandList->Close();

	if (FAILED(hr))
	{
		Running = false;
	}

}
void WaitForPreviousFrame() {

	HRESULT hr;
	
	
	frameIndex = swapChain->GetCurrentBackBufferIndex();
	
	

	if (fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent))
	{

		hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);

		if (FAILED(hr))
		{
			Running = false;
		}

		WaitForSingleObject(fenceEvent, INFINITE);
	}

	fenceValue[frameIndex]++;
}
void Cleanup() {

	tempBool = true;
	for (int i = 0; i < frameBufferCount; i++)
	{
		frameIndex = i;

		WaitForPreviousFrame();
	}

	BOOL fs = false;

	if (swapChain->GetFullscreenState(&fs, NULL))
		swapChain->SetFullscreenState(false, NULL);

	SAFE_RELEASE(device);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(commandQueue);
	SAFE_RELEASE(rtvDescriptorHeap);
	SAFE_RELEASE(commandList);
	

	for (int i = 0; i < frameBufferCount; ++i)
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocator[i]);
		SAFE_RELEASE(fence[i])
	};
	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(vertexBuffer);
}





void Render() {

	HRESULT hr;
	UpdatePipeline();

	ID3D12CommandList* ppCommandLists[] = { commandList };


	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);

	if (FAILED(hr))
	{
		Running = false;
	}

	hr = swapChain->Present(0, 0);

	if (FAILED(hr))
	{
		Running = false;
	}
}



void CheckRayTracingSupport()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};

	HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));

	if (FAILED(hr))
	{
		MessageBox(hwnd, L"Unable to check feature support", L"Error", MB_OK);
	}

	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
	{
		MessageBox(hwnd, L"Unable to support raytracing", L"Error", MB_OK);

	}
}


AccelerationStructureBuffers CreateBottomLevelAS(std::vector < std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers)
{

	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	// add all vertex buffers


	for (const auto& buffer : vVertexBuffers)
	{
		bottomLevelAS.AddVertexBuffer(buffer.first.Get(), 0, buffer.second, sizeof(Vertex), 0, 0);
	}

	//Creating BLAS requires "scratch space" for temporary info, size of scratch memory depends on scene complexity

	UINT64 scratchSizeInbytes = 0;

	//final AS needs to be stored, similar to existing vertex buffers. Size of AS depends on scene complexity

	UINT64 resultSizeInBytes = 0;

	bottomLevelAS.ComputeASBufferSizes(device, false, &scratchSizeInbytes, &resultSizeInBytes);

	// Allocate necessary buffers directly on the default heap (GPU)

	AccelerationStructureBuffers buffers;

	buffers.pScratch = nv_helpers_dx12::CreateBuffer(device, scratchSizeInbytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, nv_helpers_dx12::kDefaultHeapProps);

	buffers.pResult = nv_helpers_dx12::CreateBuffer(device, resultSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);


	// Finally we can build the acceleration structure NOTE: THIS CALL INTEGRATES A RESOURCE BARRIER ON THE GENERATED AS
	// This is done so that the BLAS can be used to compute a TLAS after this method.

	bottomLevelAS.Generate(commandList, buffers.pScratch.Get(), buffers.pResult.Get(), false, nullptr);


	return buffers;

}

void CreateTopLevelAS(const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances ) {

	// INSTANCES IS PAIR OF BLAS AND MATRIX OF THE INSTANCE

	for (size_t i = 0; i < instances.size(); i++)
	{
		topLevelASGenerator.AddInstance(
			instances[i].first.Get(),
			instances[i].second,
			static_cast<UINT>(i),
			static_cast<UINT>(0)
			);
	}


	// Just like BLAS requires scratch space in addition to actual AS
	// Unlike BLAS with TLAS instance descriptors need to be stored in GPU memory
	// this call outputs memory requirements for each (scratch, results, instance descriptors) so that the correct amount of memory can be allocated


	UINT64 scratchSize, resultSize, instanceDescSize;

	topLevelASGenerator.ComputeASBufferSizes(device, true, &scratchSize, &resultSize, &instanceDescSize);

	topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(device, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nv_helpers_dx12::kDefaultHeapProps);

	topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(device, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);

	topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(device, instanceDescSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	topLevelASGenerator.Generate(commandList, topLevelASBuffers.pScratch.Get(), topLevelASBuffers.pResult.Get(), topLevelASBuffers.pInstanceDesc.Get());




}

void CreateAccelerationStructures()
{
	HRESULT hr;

	AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS({ { vertexBuffer, 3 } });


	instances = { {bottomLevelBuffers.pResult, DirectX::XMMatrixIdentity()} };

	CreateTopLevelAS(instances);

	commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { commandList };

	commandQueue->ExecuteCommandLists(1, ppCommandLists);

	fenceValue[frameIndex]++;

	commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);

	fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
	
	WaitForSingleObject(fenceEvent, INFINITE);

	hr = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);


	if (FAILED(hr))
	{
		MessageBox(hwnd, L"Unable to reset command allocator in CreateAccelerationStructures", L"Error", MB_OK);
		return;
	}


	bottomLevelAS = bottomLevelBuffers.pResult.Get();

}

ComPtr<ID3D12RootSignature> CreateRayGenSignature()
{

	nv_helpers_dx12::RootSignatureGenerator rsc;

	rsc.AddHeapRangesParameter(
		{
			{
				0,/* u0*/
				1, /*descriptor*/
				0, /*Register space 0*/
				D3D12_DESCRIPTOR_RANGE_TYPE_UAV, /*UAV representing the output buffer*/
				0 /*where UAV is defined */
			},
		{
			0,/*t0*/
			1,
			0,
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV, /*TLAS*/
			1
		} });

	return rsc.Generate(device, true);
}

ComPtr<ID3D12RootSignature> CreateHitSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;

	//rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV);


	return rsc.Generate(device, true);
}

ComPtr<ID3D12RootSignature> CreateMissSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;

	return rsc.Generate(device, true);
}


void CreateRaytracingPipeline()
{
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(device);


	rayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"RayGen.hlsl");
	missLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Miss.hlsl");
	hitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Hit.hlsl");



	pipeline.AddLibrary(rayGenLibrary.Get(), { L"RayGen"});

	pipeline.AddLibrary(missLibrary.Get(), { L"Miss"});

	pipeline.AddLibrary(hitLibrary.Get(), { L"ClosestHit"});


	//RAYGEN
	rayGenSignature = CreateRayGenSignature();
	//MISS
	missSignature = CreateMissSignature();
	//HIT
	hitSignature = CreateHitSignature();


	/// <summary>
	/// 3 different shaders can be executed to obtain intersections
	///
	///	an intersection shader is called:
	///	- when hitting bounding box of geometry
	///	- any-hit shader
	///	- closest hit shader invoked on intersection point near ray origin
	///
	///	For triangles in DX12 an intersection shader is built in, an empty any-hit shader is defined by default. 
	/// </summary>
	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");
	
	// Associate rootsignature with corresponding shader

	//
	//

	pipeline.AddRootSignatureAssociation(rayGenSignature.Get(), {L"RayGen"});

	pipeline.AddRootSignatureAssociation(missSignature.Get(), {L"Miss"});


	pipeline.AddRootSignatureAssociation(hitSignature.Get(), {L"HitGroup"});


	pipeline.SetMaxPayloadSize(4 * sizeof(float)); /// RGB + DISTANCE
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // Barycentric coordinates


	// Raytracing can shoot rays from existing points leading to nested TraceRay function calls.

	// Trace Depth: 1 means only primary/initial rays are taken into account
	// Recursion should be kept to a minimum

	//path tracing algorithms can be flattened into a simple loop in ray generation

	pipeline.SetMaxRecursionDepth(1);

	rtStateObject = pipeline.Generate();

	ThrowIfFailed(rtStateObject->QueryInterface(IID_PPV_ARGS(&rtStateObjectProps)),L"Unable to create raytracing pipeline");

}


void CreateRaytracingOutputBuffer()
{
	D3D12_RESOURCE_DESC resDesc{ };

	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	/// Backbuffer is actually formatted as DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
	///	sRGB formats like the backbuffer are unable to be used with UAVs,
	///	for accuracy we convert to sRGB ourselves in the shader


	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = Width;
	resDesc.Height = Height;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;

	ThrowIfFailed(device->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&outputResource)),L"Unable to create image output buffer");




}
void CreateShaderResourceheap()
{
	srvUAVHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvUAVHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};

	// The Create X view methods write the view info directly into srvHandle
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	device->CreateUnorderedAccessView(outputResource.Get(), nullptr, &UAVDesc, srvHandle);

	// Add TLAS shader resource view after raytracing output buffer


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = topLevelASBuffers.pResult->GetGPUVirtualAddress();

	device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);


}
void CreateShaderBindingTable()
{
	sbtHelper.Reset();

	D3D12_GPU_DESCRIPTOR_HANDLE srvUAVHeapHandle =
		srvUAVHeap->GetGPUDescriptorHandleForHeapStart();

	auto heapPointer = reinterpret_cast<UINT64 *>(srvUAVHeapHandle.ptr);

	sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });

	// Miss and hit shaders do not access external resources, they communicate results through the ray payload instead.
	sbtHelper.AddMissProgram(L"Miss", {});

	// add triangle shader
	sbtHelper.AddHitGroup(L"HitGroup", {});


	uint32_t sbtSize = sbtHelper.ComputeSBTSize();

	sbtStorage = nv_helpers_dx12::CreateBuffer(device, sbtSize, D3D12_RESOURCE_FLAG_NONE,
	                                           D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	if(!sbtStorage)
	{
		throw std::logic_error("Could not allocate the shader binding table");
	}

	sbtHelper.Generate(sbtStorage.Get(), rtStateObjectProps.Get());

}
void OnKeyUp(UINT8 key)
{
	if (key == VK_UP)
	{
		m_raster = !m_raster;
	}
}