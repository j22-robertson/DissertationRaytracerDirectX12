#include "stdafx.h"

struct Vertex {
	DirectX::XMFLOAT3 pos;
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


	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//Update loop goes here
		}
	}
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
				adapterIndex++;
			continue;
		}

		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);

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


	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], NULL, IID_PPV_ARGS(&commandList));

	if (FAILED(hr))
	{
		MessageBox(0, L"Failed to create Command List", L"Error", MB_OK);
		return false;
	}

	commandList->Close();

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
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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
		{{0.0f,0.5f,0.5f}},
		{{0.5f,-0.5f,0.5f} },
		{{-0.5f,-0.5f,0.5f}}
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

	ID3D12Resource* vBufferUploadHeap;


	//CREATE UPLOAD HEAP
	// USED TO UPLOAD DATA TO GPU, CPU WRITES GPU READS

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


	hr = commandList->Reset(commandAllocator[frameIndex], NULL);

	if (FAILED(hr))
	{
		Running = false;
	}


	///TODO: fix l-value for rb and rb2

	auto rb = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &rb);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	const float clearColor[] = { 0.0f,0.2f,0.4f,1.0f };


	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	

	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->DrawInstanced(3, 1, 0, 0);


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
	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(vertexBuffer);

	for (int i = 0; i < frameBufferCount; ++i)
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocator[i]);
		SAFE_RELEASE(fence[i])
	};
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
