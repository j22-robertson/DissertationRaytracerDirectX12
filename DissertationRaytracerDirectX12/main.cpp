#include "stdafx.h"
#include "DXRHelper.h"
#include "BottomLevelASGenerator.h"
#include "RaytracingPipelineGenerator.h"
#include "RootSignatureGenerator.h"

//TODO: MOVE OUT OF MAIN
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

//#include "imgui.h"

std::string inputfile = "teapot.obj.txt";
tinyobj::ObjReaderConfig reader_config;
tinyobj::ObjReader reader;


UINT teapotVertNumber;
UINT teapotIndexNumber;

// See vertices for model loading




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
			UpdateCameraBuffer();
			game_time++;
			instances[0].second = DirectX::XMMatrixRotationAxis({ 0.f, 1.0f, 0.f
				}, static_cast<float>(game_time) / 50.0f) * DirectX::XMMatrixTranslation(0.f, 0.1f* cosf(game_time / 20.0f),0.0f);
			UpdatePerInstanceProperties();
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



	CD3DX12_ROOT_PARAMETER constantParameter;
	CD3DX12_DESCRIPTOR_RANGE range;

	range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	constantParameter.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_PARAMETER matricesParameter;
	CD3DX12_DESCRIPTOR_RANGE matricesRange;

	matricesRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 1);
	matricesParameter.InitAsDescriptorTable(1, &matricesRange, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_PARAMETER indexParameter;
	indexParameter.InitAsConstants(1, 1);

	std::vector<CD3DX12_ROOT_PARAMETER> parameters = { constantParameter,matricesParameter,indexParameter };


	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(static_cast<UINT>(parameters.size()),parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));

	if (FAILED(hr))
	{
		return false;
	}




	if (!reader.ParseFromFile(inputfile, reader_config))
	{
		if (!reader.Error().empty())
		{
			std::printf( "unable to read file");

		}
	}
	//Teapot only has ONE shape/mesh, expand with multiple meshes

	auto& attribs = reader.GetAttrib();
	auto& shapes = reader.GetShapes();

	auto importIndices = shapes[0].mesh.indices;


	std::vector<UINT> teapotIndices;
	for (auto index : importIndices)
	{
		teapotIndices.push_back(static_cast<UINT>(index.vertex_index));
		


	}



	//auto tpIbufferSize = teapotIndices.size();


///	attribs.vertices.size();

	teapotIndexNumber = teapotIndices.size();

	std::vector<Vertex> teapotVertices;

	if (!shapes.empty())
	{
		std::printf("Successfully imported model");
	}

	float prescale = 0.1;

	/*
	for (size_t i = 0; i < shapes[0].mesh.num_face_vertices.size(); i++)
	{
		size_t faceNum = size_t(shapes[0].mesh.num_face_vertices[i]);

		
		for (size_t v = 0; v < faceNum; v++)
		{
			Vertex tempvert = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};

			tempvert.pos.x = (attribs.vertices[v* 3 + 0] * prescale);
			tempvert.pos.y = (attribs.vertices[v * 3 + 1]* prescale);
			tempvert.pos.z = (attribs.vertices[v * 3 + 2]* prescale);

			tempvert.color.x = 1.0;
			tempvert.color.w = 1.0;


			teapotVertices.push_back(tempvert);
			//tinyobj::index_t = shapes[0].mesh.indices[i+v]
			//attribs.vertices[3*size_t()]




		}

		}*/

	for (size_t i = 0; i < attribs.vertices.size()/3; i++ )
	{



		Vertex tempvert = { 0.0,0.0,0.0,0.0,0.0,.0,.0 };	

		tempvert.pos.x = (attribs.vertices[i * 3 + 0] * prescale);
		tempvert.pos.y = (attribs.vertices[i * 3 + 1] * prescale);
		tempvert.pos.z = (attribs.vertices[i * 3 + 2] * prescale);

		tempvert.color.x = 1.0;
		tempvert.color.y = 0.0;
		tempvert.color.z = 0.0;
		tempvert.color.w = 1.0;


		teapotVertices.push_back(tempvert);

	}
	teapotVertNumber = teapotVertices.size();

	if (!teapotVertices.empty())
	{
		printf("Success");
	}

	int vBufferTeapotSize = teapotVertNumber * sizeof(Vertex);

	const UINT iBufferTeapotSize= static_cast<UINT>(teapotIndexNumber) * sizeof(UINT);



	//// TEMP TRIANGLE SETUP

	
	//triangle
	/*
	Vertex vList[] = {
	{ 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
	{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	};

	//Square/Rect

	Vertex vListRect[] = {
		{-0.5, 0.5, 0.5, 1.0,0.0,0.0,1.0},
		{0.5,-0.5,0.5,0.0,1.0,0.0,1.0},
		{-0.5,-0.5,0.5,0.0,0.0,1.0,1.0},
		{0.5,0.5,0.5,1.0,0.0,1.0,1.0}
	};


	std::vector<UINT> indices={0,1,2,0,3,1};


	int vBufferRectSize = sizeof(vListRect);
	
	const UINT iBufferSize = static_cast<UINT>(indices.size()) * sizeof(UINT);


	int vBufferSize = sizeof(vListRect);
*/
	//FIX LVLALUE
	auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resource_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vBufferTeapotSize);


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

	auto vbuffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vBufferTeapotSize);

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

	vertexData.pData = reinterpret_cast<BYTE*>(teapotVertices.data());
	vertexData.RowPitch = vBufferTeapotSize;
	vertexData.SlicePitch = vBufferTeapotSize;



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

	

	CD3DX12_HEAP_PROPERTIES iBufferHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(iBufferTeapotSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&iBufferHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferResource,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&indexBuffer)
	),L"Unable to create Index Buffer for Rect");

	UINT8* pIndexDataBegin;
	D3D12_RANGE readRange = { 0,0 };

	ThrowIfFailed(indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)),L"Unable to map index buffer");
	memcpy(pIndexDataBegin, teapotIndices.data(), iBufferTeapotSize);
	indexBuffer->Unmap(0, nullptr);

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT,
	indexBufferView.SizeInBytes = iBufferTeapotSize;
	
	CreatePlaneVB();




	commandList->ResourceBarrier(1, &rbtemp);


	//CreatePlaneVB();

	CheckRayTracingSupport();

	CreateAccelerationStructures();


	CreateRaytracingPipeline();

	CreatePerInstancePropertiesBuffer();
	CreateCameraBuffer();

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
	vertexBufferView.SizeInBytes = vBufferTeapotSize;

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


	createDepthBuffer();


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

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	if (m_raster)
	{
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0, 0.0, nullptr);
		std::vector<ID3D12DescriptorHeap*> heaps = { constHeap.Get() };
		commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
		commandList->SetGraphicsRootDescriptorTable(
			0, constHeap->GetGPUDescriptorHandleForHeapStart());

		const float clearColor[] = { 0.0f,0.2f,0.4f,1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D12_GPU_DESCRIPTOR_HANDLE handle = constHeap->GetGPUDescriptorHandleForHeapStart();
		commandList->SetGraphicsRootDescriptorTable(0, handle);
		commandList->SetGraphicsRootDescriptorTable(1, handle);

	//	commandList->SetGraphicsRoot32BitConstant(2, 0, 0);

		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->IASetIndexBuffer(&indexBufferView);
		commandList->DrawIndexedInstanced(teapotIndexNumber, instances.size()-1, 0, 0, 0);

		commandList->IASetVertexBuffers(0, 1, &planeBufferview);
		commandList->DrawInstanced(6, 1, 0, 0);

	}
	else {

		CreateTopLevelAS(instances, true);
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

void createDepthBuffer()
{
	dsvHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 1,D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);

	D3D12_HEAP_PROPERTIES depthHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);


	D3D12_RESOURCE_DESC depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Width, Height, 1, 1);

	depthResourceDesc.Flags  |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	CD3DX12_CLEAR_VALUE depthOptimizedClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
	ThrowIfFailed(device->CreateCommittedResource(
		&depthHeapProps, D3D12_HEAP_FLAG_NONE, &depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, IID_PPV_ARGS(&depthBuffer)),L"Unable to create depth buffer resource");
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());
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


AccelerationStructureBuffers CreateBottomLevelAS(std::vector < std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>,uint32_t>> vIndexBuffers)
{

	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	// add all vertex buffers


	for (size_t i = 0; i < vVertexBuffers.size(); i++)
	{
		if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
		{
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second, sizeof(Vertex), vIndexBuffers[i].first.Get(), 0, vIndexBuffers[i].second, nullptr, 0, true);
		}
		else {
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second, sizeof(Vertex), 0, 0);

		}
	}

	for (const auto& buffer : vVertexBuffers)
	{
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

void CreateTopLevelAS(const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly) {

	// INSTANCES IS PAIR OF BLAS AND MATRIX OF THE INSTANCE

	if (!updateOnly)
	{
		for (size_t i = 0; i < instances.size(); i++)
		{

			topLevelASGenerator.AddInstance(
				instances[i].first.Get(),
				instances[i].second,
				static_cast<UINT>(i),
				static_cast<UINT>(2 * i)
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
	}

	topLevelASGenerator.Generate(commandList,
		topLevelASBuffers.pScratch.Get(),
		topLevelASBuffers.pResult.Get(),
		topLevelASBuffers.pInstanceDesc.Get(),
		updateOnly,
		topLevelASBuffers.pResult.Get());




}

void CreateAccelerationStructures()
{
	HRESULT hr;

	AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS({ { vertexBuffer, teapotVertNumber } },{{indexBuffer, teapotIndexNumber}});

	AccelerationStructureBuffers planeBottomLevelBuffer = CreateBottomLevelAS({{planeBuffer.Get(), 6}},{});

	instances = { {bottomLevelBuffers.pResult, DirectX::XMMatrixIdentity()},
		{bottomLevelBuffers.pResult, DirectX::XMMatrixTranslation(-1.0,0,0)},
		{bottomLevelBuffers.pResult, DirectX::XMMatrixTranslation(1.0,0,0)},
		{planeBottomLevelBuffer.pResult, DirectX::XMMatrixTranslation(0,0,0)} };

	CreateTopLevelAS(instances, false);

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
			1,
		},
		{
		0,
		1,
		0,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		2,
		}
		});

	return rsc.Generate(device, true);
}

ComPtr<ID3D12RootSignature> CreateHitSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;

	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV,0/*t0*/);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1/*t1*/);
	rsc.AddHeapRangesParameter({{
		2 /*t2*/,
		1,
		0,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1/*2nd heap slot*/
		},
		
		{
			3/*t3*/,
		1,
		0,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		3
		} /* Per Instance Data*/
		});



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
	shadowLibrary = nv_helpers_dx12::CompileShaderLibrary(L"ShadowHit.hlsl");




	pipeline.AddLibrary(rayGenLibrary.Get(), { L"RayGen"});

	pipeline.AddLibrary(missLibrary.Get(), { L"Miss"});

	pipeline.AddLibrary(hitLibrary.Get(), { L"ClosestHit",L"PlaneClosestHit"});

	pipeline.AddLibrary(shadowLibrary.Get(),{L"ShadowClosestHit",L"ShadowMiss"});





	//RAYGEN
	rayGenSignature = CreateRayGenSignature();
	//MISS
	missSignature = CreateMissSignature();
	//HIT
	hitSignature = CreateHitSignature();
	//ShadowRays
	shadowHitSignature = CreateHitSignature();


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
	pipeline.AddHitGroup(L"PlaneHitGroup", L"PlaneClosestHit");


	//shadows
	pipeline.AddHitGroup(L"ShadowHitGroup",L"ShadowClosestHit");
	// Associate rootsignature with corresponding shader

	//
	//


	pipeline.AddRootSignatureAssociation(rayGenSignature.Get(), {L"RayGen"});

	pipeline.AddRootSignatureAssociation(shadowHitSignature.Get(), {L"ShadowHitGroup"});
	pipeline.AddRootSignatureAssociation(missSignature.Get(), {L"Miss",L"ShadowMiss"});


	pipeline.AddRootSignatureAssociation(hitSignature.Get(), {L"HitGroup",L"PlaneHitGroup"});









	pipeline.SetMaxPayloadSize(7 * sizeof(float)); /// RGB + DISTANCE + canReflect + maxdepth + currentdepth
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // Barycentric coordinates


	// Raytracing can shoot rays from existing points leading to nested TraceRay function calls.

	// Trace Depth: 1 means only primary/initial rays are taken into account
	// Recursion should be kept to a minimum

	//path tracing algorithms can be flattened into a simple loop in ray generation

	pipeline.SetMaxRecursionDepth(4);

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
	srvUAVHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

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

	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};

	cbvDesc.BufferLocation = cameraBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cameraBufferSize;
	device->CreateConstantBufferView(&cbvDesc, srvHandle);

	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC perInstanceViewDesc;
	perInstanceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	perInstanceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	perInstanceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	perInstanceViewDesc.Buffer.FirstElement = 0;
	perInstanceViewDesc.Buffer.NumElements = static_cast<UINT>(instances.size());
	perInstanceViewDesc.Buffer.StructureByteStride = sizeof(PerInstanceProperties);
	perInstanceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(perInstancePropertiesBuffer.Get(), &perInstanceViewDesc, srvHandle);
	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


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
	sbtHelper.AddMissProgram(L"ShadowMiss", {});






	for (int i = 0; i < instances.size()-1; i++)
	{
		sbtHelper.AddHitGroup(L"HitGroup",{(void*)vertexBuffer->GetGPUVirtualAddress(),(void*)indexBuffer->GetGPUVirtualAddress()});
		sbtHelper.AddHitGroup(L"ShadowHitGroup", {});
	}

	sbtHelper.AddHitGroup(L"PlaneHitGroup", { (void*)planeBuffer->GetGPUVirtualAddress(), nullptr,heapPointer});
	sbtHelper.AddHitGroup(L"ShadowHitGroup", {});
	// add triangle shader
//	sbtHelper.AddHitGroup(L"HitGroup", {(void*)(vertexBuffer->GetGPUVirtualAddress()),(void*)(indexBuffer->GetGPUVirtualAddress()),(void*)globalConstBuffer->GetGPUVirtualAddress()});


	
	

	uint32_t sbtSize = sbtHelper.ComputeSBTSize();

	sbtStorage = nv_helpers_dx12::CreateBuffer(device, sbtSize, D3D12_RESOURCE_FLAG_NONE,
	                                           D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	if(!sbtStorage)
	{
		throw std::logic_error("Could not allocate the shader binding table");
	}

	sbtHelper.Generate(sbtStorage.Get(), rtStateObjectProps.Get());

}
void CreateCameraBuffer()
{


	uint32_t nbMatrix = 4; // View, Perspective, View inv, Perspective inv

	cameraBufferSize = nbMatrix * sizeof(DirectX::XMMATRIX);
	cameraBuffer = nv_helpers_dx12::CreateBuffer(device, cameraBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);


	/// EDITED HEAP FROM 1 TO 2 TO MAKE ROOM FOR PER INSTANCE PROPERTIES
	// TODO: Create a heap allocator class to allocate memory for the camera, instances and other const buffer properties like light data
	constHeap = nv_helpers_dx12::CreateDescriptorHeap(
		device, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true
	);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};

	cbvDesc.BufferLocation = cameraBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cameraBufferSize;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = constHeap->GetCPUDescriptorHandleForHeapStart();

	device->CreateConstantBufferView(&cbvDesc, srvHandle);



	// TODO: Move creation of PerInstanceProperties view out of this function
	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(instances.size());
	srvDesc.Buffer.StructureByteStride = sizeof(PerInstanceProperties);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	

	device->CreateShaderResourceView(perInstancePropertiesBuffer.Get(), &srvDesc, srvHandle);


}
void UpdateCameraBuffer()
{
	/*std::vector<DirectX::XMMATRIX> matrices(4);

	// init view matrix

	DirectX::XMVECTOR cameraEye = DirectX::XMVectorSet(1.5f, 1.5f, 1.5f, 0.0f);

	DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f,0.0f,0.0f,0.0f);

	DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0, 1.0f, 0.0f, 0.0f);

	matrices[0] = DirectX::XMMatrixLookAtRH(cameraEye, At, Up);

	float fovAngleY = 45.0f * DirectX::XM_PI / 180.0f;

	float aspectRatio = static_cast<float>(Width) / static_cast<float>(Height);

	matrices[1] = DirectX::XMMatrixPerspectiveFovRH(fovAngleY,aspectRatio, 0.1f, 1000.0f);


	// RAYS ARE DEFINED IN CAMERA SPACE AND TRANSFORMED TO WORLD SPACE
	// WE NEED TO STORE INVERSE MATRICES

	DirectX::XMVECTOR det;

	matrices[2] = DirectX::XMMatrixInverse(&det, matrices[0]);
	matrices[3] = DirectX::XMMatrixInverse(&det, matrices[1]);*/
	//cameraController.UpdateCamera(Width, Height);

	auto matrices = cameraController.GetCameraData(Width, Height);

	std::uint8_t* pdata;

	ThrowIfFailed(cameraBuffer->Map(0, nullptr, (void**)&pdata), L"Failed to map camera buffer");

	memcpy(pdata, matrices.data(), cameraBufferSize);

	cameraBuffer->Unmap(0, nullptr);
}
void CreateGlobalConstantBuffer()
{
	DirectX::XMVECTOR bufferData[] =
	{
	DirectX::XMVECTOR{1.0f, 0.0f, 0.0f, 1.0f},
	DirectX::XMVECTOR{0.7f, 0.4f, 0.0f, 1.0f},
	DirectX::XMVECTOR{0.4f, 0.7f, 0.0f, 1.0f},

	// B
	DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 1.0f},
	DirectX::XMVECTOR{0.0f, 0.7f, 0.4f, 1.0f},
	DirectX::XMVECTOR{0.0f, 0.4f, 0.7f, 1.0f},

	// C
	DirectX::XMVECTOR{0.0f, 0.0f, 1.0f, 1.0f},
	DirectX::XMVECTOR{0.4f, 0.0f, 0.7f, 1.0f},
	DirectX::XMVECTOR{0.7f, 0.0f, 0.4f, 1.0f},


	};

	globalConstBuffer = nv_helpers_dx12::CreateBuffer(
		device,
		sizeof(bufferData),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nv_helpers_dx12::kUploadHeapProps
		);

	uint8_t* pData;

	ThrowIfFailed(globalConstBuffer->Map(0, nullptr, (void**)&pData),L"Unable to create global const buffer");
	memcpy(pData, bufferData, sizeof(bufferData));
	globalConstBuffer->Unmap(0, nullptr);
}
void CreatePerInstanceBuffer()
{
	DirectX::XMVECTOR bufferData[] =
	{
	DirectX::XMVECTOR{1.0f, 0.0f, 0.0f, 1.0f},
	DirectX::XMVECTOR{0.7f, 0.4f, 0.0f, 1.0f},
	DirectX::XMVECTOR{0.4f, 0.7f, 0.0f, 1.0f},

	// B
	DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 1.0f},
	DirectX::XMVECTOR{0.0f, 0.7f, 0.4f, 1.0f},
	DirectX::XMVECTOR{0.0f, 0.4f, 0.7f, 1.0f},

	// C
	DirectX::XMVECTOR{0.0f, 0.0f, 1.0f, 1.0f},
	DirectX::XMVECTOR{0.4f, 0.0f, 0.7f, 1.0f},
	DirectX::XMVECTOR{0.7f, 0.0f, 0.4f, 1.0f},
	};



	
	perInstanceConstantBuffers.resize(3);

	int i(0);
	for (auto& buffer : perInstanceConstantBuffers)
	{
		const uint32_t bufferSize = sizeof(DirectX::XMVECTOR) * 3;
		buffer = nv_helpers_dx12::CreateBuffer(device, bufferSize, D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

		uint8_t* pData;

		ThrowIfFailed(buffer->Map(0, nullptr, (void**)&pData), L"Unable to load per instance constant buffer");
		memcpy(pData, &bufferData[i * 3], bufferSize);
		buffer->Unmap(0, nullptr);
		i++;
	}

}
void CreatePlaneVB()
{
	
	Vertex planeVertices[] = {
	 {-1.5f, -.8f, 01.5f, 1.0f, 1.0f, 1.0f, 1.0f}, // 0
	 {-1.5f, -.8f, -1.5f, 1.0f, 1.0f, 1.0f, 1.0f}, // 1
	 {01.5f, -.8f, 01.5f, 1.0f, 1.0f, 1.0f, 1.0f}, // 2
	 {01.5f, -.8f, 01.5f, 1.0f, 1.0f, 1.0f, 1.0f}, // 2
	 {-1.5f, -.8f, -1.5f, 1.0f, 1.0f, 1.0f, 1.0f}, // 1
	 {01.5f, -.8f, -1.5f, 1.0f, 1.0f, 1.0f, 1.0f}  // 4
	};

	const UINT planeBufferSize= sizeof(planeVertices);

	///Todo: USING UPLOAD HEAPS FOR VBO IS NOT A GOOD IDEA. Why?
	/// - The upload heap will be marshalled over every time the GPU needs it
	///The upload heap is used here for temporary code simplicity
	/// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	/// READ UP ON DEFAULT HEAP USAGE
	/// 
	CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(planeBufferSize);

	ThrowIfFailed(device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&planeBuffer)),L"Unable to create Plane VBO");

	UINT8 *pVertexDatabegin;
	CD3DX12_RANGE readRange(0, 0);

	ThrowIfFailed(planeBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDatabegin)),L"Unable to create plane vertex buffer");

	memcpy(pVertexDatabegin, planeVertices, sizeof(planeVertices));

	planeBuffer->Unmap(0, nullptr);

	planeBufferview.BufferLocation = planeBuffer->GetGPUVirtualAddress();
	planeBufferview.StrideInBytes = sizeof(Vertex);
	planeBufferview.SizeInBytes = planeBufferSize;

}
void OnKeyUp(UINT8 key)
{
	if (key == VK_UP)
	{
		m_raster = !m_raster;
	}
	if (key == VK_DOWN)
	{
		cameraController.MoveForward();
	}
}



void CreatePerInstancePropertiesBuffer()
{
	std::uint32_t buffersize = ROUND_UP(static_cast<std::uint32_t>(instances.size()) * sizeof(PerInstanceProperties), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	perInstancePropertiesBuffer = nv_helpers_dx12::CreateBuffer(device, buffersize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);


}

void UpdatePerInstanceProperties()
{

	PerInstanceProperties* current = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(perInstancePropertiesBuffer->Map(0, &readRange, reinterpret_cast<void**>(&current)), L"Unable to map instance properties");
	for (const auto& instance : instances)
	{
		current->objectToWorld = instance.second;
		DirectX::XMMATRIX upper3x3 = instance.second;
		upper3x3.r[0].m128_f32[3] = 0.f;
		upper3x3.r[1].m128_f32[3] = 0.f;
		upper3x3.r[2].m128_f32[3] = 0.f;
		upper3x3.r[3].m128_f32[0] = 0.f;
		upper3x3.r[3].m128_f32[1] = 0.f;
		upper3x3.r[3].m128_f32[2] = 0.f;
		upper3x3.r[3].m128_f32[3] = 1.f;
		DirectX::XMVECTOR det;
		current->objectToWorldNormal = XMMatrixTranspose(XMMatrixInverse(&det, upper3x3));
		current++;
	}
	perInstancePropertiesBuffer->Unmap(0, nullptr);
}
