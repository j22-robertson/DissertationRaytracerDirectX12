#include "RenderApplication.h"

#include "BottomLevelASGenerator.h"

#include "RaytracingPipelineGenerator.h"
#include "RootSignatureGenerator.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <functional>
#include "DXRHelper.h"
#include "tiny_obj_loader.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_win32.h"
#include "Imgui/imgui_impl_dx12.h"

void RenderApplication::createDepthBuffer()
{
	dsvHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);

	D3D12_HEAP_PROPERTIES depthHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

//	DirectX::LoadFromDDSFile()
	D3D12_RESOURCE_DESC depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Width, Height, 1, 1);

	depthResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;


	CD3DX12_CLEAR_VALUE depthOptimizedClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	ThrowIfFailed(device->CreateCommittedResource(
		&depthHeapProps, D3D12_HEAP_FLAG_NONE, &depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, IID_PPV_ARGS(&depthBuffer)), L"Unable to create depth buffer resource");
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc,
	dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void RenderApplication::CreateCameraBuffer()
{



	uint32_t nbMatrix = 4; // View, Perspective, View inv, Perspective inv

	cameraBufferSize = nbMatrix * sizeof(DirectX::XMMATRIX);
	cameraBuffer = nv_helpers_dx12::CreateBuffer(device, cameraBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);


	/// EDITED HEAP FROM 1 TO 2 TO MAKE ROOM FOR PER INSTANCE PROPERTIES
	// TODO: Create a heap allocator class to allocate memory for the camera, instances and other const buffer properties like light data
	constHeap = nv_helpers_dx12::CreateDescriptorHeap(
		device, 5, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true
	);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};

	cbvDesc.BufferLocation = cameraBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cameraBufferSize;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = constHeap->GetCPUDescriptorHandleForHeapStart();

	device->CreateConstantBufferView(&cbvDesc, srvHandle);


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
	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	
	ImGui_ImplDX12_Init(device, 3, DXGI_FORMAT_R8G8B8A8_UNORM,
		constHeap.Get(),
		// You'll need to designate a descriptor from your descriptor heap for Dear ImGui to use internally for its font texture's SRV
		constHeap->GetCPUDescriptorHandleForHeapStart(),
		constHeap->GetGPUDescriptorHandleForHeapStart());


}

void RenderApplication::UpdateCameraBuffer()
{
	auto matrices = cameraController.GetCameraData(Width, Height);

	std::uint8_t* pdata;

	ThrowIfFailed(cameraBuffer->Map(0, nullptr, (void**)&pdata), L"Failed to map camera buffer");

	memcpy(pdata, matrices.data(), cameraBufferSize);

	cameraBuffer->Unmap(0, nullptr);
}

void RenderApplication::OnKeyUp(UINT8 key)
{


	switch (key)
	{
	case VK_UP:
	{
		cameraController.MoveForward();
		return;
	}
	case VK_DOWN:
	{
		m_raster = !m_raster;
		return;
	}

	case VK_LEFT: 
	{
			cameraController.MoveLeft();
			return;
	}

	case VK_RIGHT:
		{
		cameraController.MoveRight();
		return;
		}

	 default:
	{
		return;
	}
	}
	/*
	if(key == VK_UP)
	{
		cameraController.MoveForward();
		return;
	}
	if(key == VK_DOWN)
	{
		m_raster = !m_raster;
		return;
	}
	/*
	switch(key)
	{
		case VK_UP: cameraController.MoveForward();

	//	case VK_DOWN: cameraController.moveBack();

	//	case VK_LEFT: cameraController.MoveLeft();

	//	case VK_RIGHT: cameraController.MoveRight();

		case VK_ACCEPT: m_raster = !m_raster;

	default: return;
	}
	*/

	/*
	if (key == VK_UP)
	{
		m_raster =!m_raster;
	}
	if (key == VK_DOWN)
	{
		cameraController.MoveForward();
	}*/
}

void RenderApplication::CreatePlaneVB()
{
	Vertex planeVertices[] = {
 {-1.5f, -.8f, 01.5f, 1.0f, 1.0f, 1.0f, 1.0f,0.0,0.0}, // 0
 {-1.5f, -.8f, -1.5f, 1.0f, 1.0f, 1.0f, 1.0f,0.0,0.0}, // 1
 {01.5f, -.8f, 01.5f, 1.0f, 1.0f, 1.0f, 1.0f,0.0,0.0}, // 2
 {01.5f, -.8f, 01.5f, 1.0f, 1.0f, 1.0f, 1.0f,0.0,0.0}, // 2
 {-1.5f, -.8f, -1.5f, 1.0f, 1.0f, 1.0f, 1.0f,0.0,0.0}, // 1
 {01.5f, -.8f, -1.5f, 1.0f, 1.0f, 1.0f, 1.0f,0.0,0.0}  // 4
	};

	const UINT planeBufferSize = sizeof(planeVertices);

	///Todo: USING UPLOAD HEAPS FOR VBO IS NOT A GOOD IDEA. Why?
	/// - The upload heap will be marshalled over every time the GPU needs it
	///The upload heap is used here for temporary code simplicity
	/// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	/// READ UP ON DEFAULT HEAP USAGE
	/// 
	CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(planeBufferSize);

	ThrowIfFailed(device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&planeBuffer)), L"Unable to create Plane VBO");

	UINT8* pVertexDatabegin;
	CD3DX12_RANGE readRange(0, 0);

	ThrowIfFailed(planeBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDatabegin)), L"Unable to create plane vertex buffer");

	memcpy(pVertexDatabegin, planeVertices, sizeof(planeVertices));

	planeBuffer->Unmap(0, nullptr);

	planeBufferview.BufferLocation = planeBuffer->GetGPUVirtualAddress();
	planeBufferview.StrideInBytes = sizeof(Vertex);
	planeBufferview.SizeInBytes = planeBufferSize;
}

void RenderApplication::CreatePerInstanceBuffer()
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

void RenderApplication::createBackgroundBuffer()
{
//	device->CreateCommittedResource(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &backgroundColddor)
	
	backgroundColor = nv_helpers_dx12::CreateBuffer(device, sizeof(DirectX::XMVECTOR), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	 bgcol = { (float)0.9,(float)0.1,(float)0.1,0.0 };


	 std::vector<float> mybgcol = { 0.5,0.5,0.1,0.0 };
	 CD3DX12_RANGE readRange(0, 0);
//
	std::uint8_t* bgdata;
//
	ThrowIfFailed(backgroundColor->Map(0,&readRange, (void**)&bgdata), L"Failed to map background buffer");

	memcpy(bgdata,mybgcol.data(), sizeof(DirectX::XMVECTOR));

	backgroundColor->Unmap(0, nullptr);

}

void RenderApplication::UpdateBackgroundBuffer()
{
	CD3DX12_RANGE readRange(0, 0);
	//
	
	std::uint8_t* bgdata;
	//
	ThrowIfFailed(backgroundColor->Map(0, &readRange, (void**)&bgdata), L"Failed to map background buffer");

	memcpy(bgdata, &bgcolour, sizeof(DirectX::XMVECTOR));

	backgroundColor->Unmap(0, nullptr);

}

void RenderApplication::LoadEnvironmentMap(const wchar_t* filename)
{

//	data->



	image = std::make_unique<DirectX::ScratchImage>();

	ThrowIfFailed(DirectX::LoadFromHDRFile(filename, nullptr, *image), L"Unable to read metadata from " + *filename);

	ThrowIfFailed(DirectX::CreateTexture(device,image->GetMetadata(), &env_texture),L"Unable to load texture "+ *filename);
	env_texture->SetName(L"Environment Map");


	std::vector<D3D12_SUBRESOURCE_DATA> subresources;


	DirectX::PrepareUpload(device, image->GetImages(), image->GetImageCount(), image->GetMetadata(), subresources);
	const UINT64 env_buffsize = GetRequiredIntermediateSize(env_texture.Get(), 0, static_cast<unsigned int>(subresources.size()));

	auto heapprops = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffdesc = CD3DX12_RESOURCE_DESC::Buffer(env_buffsize);


	ThrowIfFailed( device->CreateCommittedResource(
		&heapprops,
		D3D12_HEAP_FLAG_NONE,
	&buffdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(textureUploadHeap.GetAddressOf())),L"Unable to create committed memory for envmap");

	textureUploadHeap->SetName(L"Environment Map Upload Heap");

	UpdateSubresources(commandList, env_texture.Get(), textureUploadHeap.Get(), 0, 0, static_cast<unsigned int>(subresources.size()),subresources.data());


	



}

void RenderApplication::LoadTextureFromFile(const wchar_t* filename, std::string name)
{


	
	auto temp_image = std::make_unique<DirectX::ScratchImage>();


	

	ThrowIfFailed(DirectX::LoadFromWICFile(filename, DirectX::WIC_FLAGS_NONE, &textureMetaData[name], *temp_image), L"Unable to read metadata from water.png");


	textures[name] = ComPtr<ID3D12Resource>();


	
	ThrowIfFailed(DirectX::CreateTexture(device, textureMetaData[name], &textures[name]), L"Unable to load texture " + *filename);

	textures[name]->SetName(filename);
	//env_texture->SetName(L"Environment Map");
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;




	DirectX::PrepareUpload(device, temp_image->GetImages(), temp_image->GetImageCount(), temp_image->GetMetadata(), subresources);
	const UINT64 texture_buffsize = GetRequiredIntermediateSize(textures[name].Get(), 0, static_cast<unsigned int>(subresources.size()));
	auto heapprops = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffdesc = CD3DX12_RESOURCE_DESC::Buffer(texture_buffsize);


	tempHeaps[name] = ComPtr<ID3D12Resource>();

	// tempheaps.emplace_back(ComPtr<ID3D12Resource>());// = ComPtr<ID3D12Resource>();

	ThrowIfFailed(device->CreateCommittedResource(
		&heapprops,
		D3D12_HEAP_FLAG_NONE,
		&buffdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(tempHeaps[name].GetAddressOf())), L"Unable to create committed memory for texture: " + *filename);

	tempHeaps[name]->SetName(L"Upload heap:" + *filename);


	UpdateSubresources(commandList, textures[name].Get(), tempHeaps[name].Get(), 0, 0, static_cast<unsigned int>(subresources.size()), subresources.data());


	auto rbtemp = CD3DX12_RESOURCE_BARRIER::Transition(textures[name].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	commandList->ResourceBarrier(1, &rbtemp);
	

}

void RenderApplication::CreatePerInstancePropertiesBuffer()
{

	std::uint32_t buffersize = ROUND_UP(static_cast<std::uint32_t>(instances.size()) * sizeof(PerInstanceProperties), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	perInstancePropertiesBuffer = nv_helpers_dx12::CreateBuffer(device, buffersize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
}

void RenderApplication::UpdatePerInstancePropertiesBuffer()
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

bool RenderApplication::InitD3D()
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


	//Check all GPU adapters
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



	timer.SetFixedTimeStep(true);
	timer.SetTargetElapsedSeconds(1.f / 60.f);

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
	rootSignatureDesc.Init(static_cast<UINT>(parameters.size()), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
		{"UV",0,DXGI_FORMAT_R32G32_FLOAT, 28,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA}
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
	modelResourceHandler->loadFromObj("teapotuv.obj", device, commandList, "TEAPOT");
	modelResourceHandler->loadFromObj("cube.obj", device, commandList, "CUBE");



	if (!reader.ParseFromFile(inputfile, reader_config))
	{
		if (!reader.Error().empty())
		{
			std::printf("unable to read file");

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
	std::vector<float> uvs;

	attribs.texcoords;


	//auto tpIbufferSize = teapotIndices.size();


///	attribs.vertices.size();

	teapotIndexNumber = teapotIndices.size();

	std::vector<Vertex> teapotVertices;

	if (!shapes.empty())
	{
		std::printf("Successfully imported model");
	}

	float prescale = 0.1;

	for (size_t i = 0; i < attribs.vertices.size() / 3; i++)
	{



		Vertex tempvert = { 0.0,0.0,0.0,0.0,0.0,.0,.0,0.0,0.0 };

		tempvert.pos.x = (attribs.vertices[i * 3 + 0]);
		tempvert.pos.y = (attribs.vertices[i * 3 + 1]);
		tempvert.pos.z = (attribs.vertices[i * 3 + 2]);

	//float e = 	attribs.texcoords[i * 3 + 0];
		

		tempvert.color.x = 1.0;
		tempvert.color.y = 0.0;
		tempvert.color.z = 0.0;
		tempvert.color.w = 1.0;

		tempvert.uv.x = attribs.texcoords[i*2 + 0];
		tempvert.uv.y = attribs.texcoords[i*2 + 1];


		teapotVertices.push_back(tempvert);

	}
	teapotVertNumber = teapotVertices.size();

	if (!teapotVertices.empty())
	{
		printf("Success");
	}


	int vBufferTeapotSize = teapotVertNumber * sizeof(Vertex);

	const UINT iBufferTeapotSize = static_cast<UINT>(teapotIndexNumber) * sizeof(UINT);



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
	auto rbtemp = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	



	CD3DX12_HEAP_PROPERTIES iBufferHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(iBufferTeapotSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&iBufferHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferResource,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&indexBuffer)
	), L"Unable to create Index Buffer for Rect");

	UINT8* pIndexDataBegin;
	D3D12_RANGE readRange = { 0,0 };

	ThrowIfFailed(indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)), L"Unable to map index buffer");
	memcpy(pIndexDataBegin, teapotIndices.data(), iBufferTeapotSize);
	indexBuffer->Unmap(0, nullptr);

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT,
		indexBufferView.SizeInBytes = iBufferTeapotSize;

	CreatePlaneVB();




	commandList->ResourceBarrier(1, &rbtemp);


	//CreatePlaneVB();

	CheckRaytracingSupport();


	CreateAccelerationStructures();


	CreateRaytracingPipeline();

	CreatePerInstancePropertiesBuffer();
	CreateCameraBuffer();
	createBackgroundBuffer();

	LoadEnvironmentMap(L"cobblestone_street_night_2k.hdr");


	auto rbtemp3 = CD3DX12_RESOURCE_BARRIER::Transition(env_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	commandList->ResourceBarrier(1, &rbtemp3);


	LoadTextureFromFile(L"BrickWall29_1K_BaseColor.png","TEST");


///	LoadTextureFromFile(L"rusty-ribbed-metal_albedo.png", "ALBEDO");
///	LoadTextureFromFile(L"rusty-ribbed-metal_metallic.png", "METAL");
//	LoadTextureFromFile(L"rusty-ribbed-metal_normal-dx.png", "NORMAL");
//	LoadTextureFromFile(L"rusty-ribbed-metal_roughness.png", "ROUGHNESS");

//	LoadTextureFromFile(L"clay-shingles1_albedo.png", "ALBEDO");
//	LoadTextureFromFile(L"clay-shingles1_metallic.png", "METAL");
//	LoadTextureFromFile(L"clay-shingles1_normal-dx.png", "NORMAL");
//	LoadTextureFromFile(L"clay-shingles1_roughness.png", "ROUGHNESS");
	//clay - shingles1_albedo
	//	LoadTextureFromFile(L"rusty-ribbed-metal_roughness.png", "ROUGHNESS");
	// Allocate memory buffer storing the RayTracing output. Has same DIMENSIONS as the target image




	LoadTextureFromFile(L"textured-aluminum_albedo.png", "ALBEDO");
	LoadTextureFromFile(L"textured-aluminum_metallic.png", "METAL");
	LoadTextureFromFile(L"textured-aluminum_normal-dx.png", "NORMAL");
	LoadTextureFromFile(L"textured-aluminum_roughness.png", "ROUGHNESS");
	CreateRaytracingOutputBuffer();





	// Create the buffer containing the results of RayTracing (always output in a UAV), and create the heap referencing the resources used by the RayTracing e.g. acceleration structures
	CreateShaderResourceheap();

	commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	CreateShaderBindingTable();
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




int RenderApplication::setup(HINSTANCE hInstance, int ShowWnd, int width, int height, bool fullscreen)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	if (!InitializeWindow(hInstance, ShowWnd, Width, Height, FullScreen))
	{
		MessageBox(0, L"Window Initializaiton - Failed", L"Error", MB_OK);
		return 1;
	}
	
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}

	if (!InitD3D())
	{
		MessageBox(0, L"Failed to initialize DX12", L"Error", MB_OK);

		Cleanup();
		return 2;
	}


	// Setup Dear ImGui context
  // Enable Keyboard Controls
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	// Setup Platform/Renderer backends



	mainloop();

	WaitForPreviousFrame();
	CloseHandle(fenceEvent);
	Cleanup();

	//Cleanup();
//	InitializeWindow(hInstance, ShowWnd, width, height, fullscreen);
//	InitD3D();

	return 0;
}

bool RenderApplication::InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool fullscreen)
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
		this);

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

	ImGui_ImplWin32_Init(hwnd);

	return true;
	//return false;
}

void RenderApplication::mainloop()
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
			 {
			/*	float size = 1.0;
		
				UpdateCameraBuffer();
				
				UpdateBackgroundBuffer();
				


				float dt = currentTime - timer.GetElapsedSeconds();
				currentTime = timer.GetElapsedSeconds();




				game_time++;
				instances[0].second = DirectX::XMMatrixRotationAxis({ 0.f, 1.0f, 0.f
					}, temprot  += rotspeed * dt) * DirectX::XMMatrixTranslation(0.f, 0.1f * cosf(game_time / 20.0f), 0.0f) * DirectX::XMMatrixScaling(size,size,size);

				instances[1].second = DirectX::XMMatrixRotationAxis({ 0.f, 1.0f, 0.f
					}, rotspeed) * DirectX::XMMatrixTranslation(23.0f/size, 0.0f , 0.0f) * DirectX::XMMatrixScaling(size, size, size);

				instances[2].second = DirectX::XMMatrixRotationAxis({ 0.f, 1.0f, 0.f
					},  rotspeed) * DirectX::XMMatrixTranslation(-23.0f / size, 0.0f, 0.0f)* DirectX::XMMatrixScaling(size, size, size);

				UpdatePerInstancePropertiesBuffer();
				Render();*/
				Tick();

				//Update loop goes here
			}
		}
	
}

void RenderApplication::UpdatePipeline()
{
		HRESULT hr;
	//	ImGui::EndFrame();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("RotationSpeed");

		ImGui::SliderFloat("Speed", &rotspeed, 0.0, 2.0);
	///	ImGui::SliderFloat("Size", &size, 1.0, 10.0);
		ImGui::End();
		ImGui::Begin("RotationSpeed");
		ImGui::Text("Framrate: %d" , timer.GetFramesPerSecond());
		ImGui::End();
		///	ImGui::SliderFloat("Size", &size, 1.0, 10.0);
		ImGui::End();
		ImGui::Render();

	WaitForPreviousFrame();

//	hr = commandAllocator[frameIndex]->Reset();

//	if (FAILED(hr))
//	{
//		Running = false;
//	}

	

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
	const float clearColor[] = { 0.0f,0.2f,0.4f,1.0f };






	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	if (m_raster)
	{
	
	
		std::vector<ID3D12DescriptorHeap*> heaps = { constHeap.Get() };
		commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
		commandList->SetGraphicsRootDescriptorTable(
			0, constHeap->GetGPUDescriptorHandleForHeapStart());
		

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
		D3D12_GPU_DESCRIPTOR_HANDLE handle = constHeap->GetGPUDescriptorHandleForHeapStart();
		commandList->SetGraphicsRootDescriptorTable(0, handle);
		commandList->SetGraphicsRootDescriptorTable(1, handle);

	//	commandList->SetGraphicsRoot32BitConstant(2, 0, 0);
	
	
		commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0, 0.0, nullptr);
		//commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

		commandList->IASetVertexBuffers(0, 1, &modelResourceHandler->getModelVertexBufferView("TEAPOT"));

	//	commandList->IASetIndexBuffer(&indexBufferView);


		//commandList->IASetIndexBuffer(&indexBufferView);
		commandList->IASetIndexBuffer(&modelResourceHandler->getModelIndexView("TEAPOT"));
		//commandList->DrawIndexedInstanced(teapotIndexNumber, instances.size()-1, 0, 0, 0);
		commandList->DrawIndexedInstanced(modelResourceHandler->getModelIndexNumber("TEAPOT"), instances.size() - 1, 0, 0, 0);

		commandList->IASetVertexBuffers(0, 1, &planeBufferview);
		commandList->DrawInstanced(6, 1, 0, 0);

		commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0, 0.0, nullptr);
	
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);


	//	ImGui::Render();
	//	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	}
	else {

		CreateTopLevelAS(instances, true);
		const float clearColor[] = { 0.0f,0.2f,0.4f,1.0f };
	//	const float clearColor[] = { 0.0f,0.2f,0.4f,1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		std::vector<ID3D12DescriptorHeap*> heaps = { srvUAVHeap.Get(), tempSamplerHeap.Get()};
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
//	commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0, 0.0, nullptr);
//	ImGui::EndFrame();
//	ImGui::Render();
//	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
//	ImGui::Render();
	//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	//TODO: Research how to handle imgui with raytracing (This works and seems correct though, simply switching our descriptor heap for rasterizing our imgui window)
	if(!m_raster)
	{
		ImGui::Render();
		
		std::vector<ID3D12DescriptorHeap*> heaps = { constHeap.Get() };
		commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
		commandList->SetGraphicsRootDescriptorTable(
			0, constHeap->GetGPUDescriptorHandleForHeapStart());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	}

//	ImGui::Render();



	auto rb2 = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &rb2);

	hr = commandList->Close();

	if (FAILED(hr))
	{
		Running = false;
	}
}

void RenderApplication::WaitForPreviousFrame()
{

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

void RenderApplication::Cleanup()
{
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
	
//	env_texture->Release();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void RenderApplication::Tick()
{
	timer.Tick([&]
		{
			update(timer);
	});

	Render();
}

void RenderApplication::update(DX::StepTimer const& step_timer)
{
	float size = 1.0;
	UpdateCameraBuffer();

	UpdateBackgroundBuffer();

	double dt =(double)currentTime - step_timer.GetElapsedSeconds();





	game_time++;
	instances[0].second = DirectX::XMMatrixRotationAxis({ 0.f, 1.0f, 0.f
		}, temprot += rotspeed * dt) * DirectX::XMMatrixTranslation(0.f, 0.1f * cosf(game_time / 20.0f), 0.0f) * DirectX::XMMatrixScaling(size, size, size);

	instances[1].second = DirectX::XMMatrixRotationAxis({ 0.f, 1.0f, 0.f
		}, rotspeed) * DirectX::XMMatrixTranslation(30.0f / size, 0.0f, 0.0f) * DirectX::XMMatrixScaling(size, size, size);

	instances[2].second = DirectX::XMMatrixRotationAxis({ 0.f, 1.0f, 0.f
		}, rotspeed) * DirectX::XMMatrixTranslation(-30.0f / size, 0.0f, 0.0f) * DirectX::XMMatrixScaling(size, size, size);

	UpdatePerInstancePropertiesBuffer();
}

void RenderApplication::Render()
{
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


void RenderApplication::CheckRaytracingSupport()
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

void RenderApplication::CreateRaytracingPipeline()
{
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(device);


	rayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"RayGen.hlsl");
	missLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Miss.hlsl");
	hitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Hit.hlsl");
	shadowLibrary = nv_helpers_dx12::CompileShaderLibrary(L"ShadowHit.hlsl");




	pipeline.AddLibrary(rayGenLibrary.Get(), { L"RayGen" });

	pipeline.AddLibrary(missLibrary.Get(), { L"Miss" });

	pipeline.AddLibrary(hitLibrary.Get(), { L"ClosestHit",L"PlaneClosestHit" });

	pipeline.AddLibrary(shadowLibrary.Get(), { L"ShadowClosestHit",L"ShadowMiss" });





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
	pipeline.AddHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");
	// Associate rootsignature with corresponding shader

	//
	//


	pipeline.AddRootSignatureAssociation(rayGenSignature.Get(), { L"RayGen" });

	pipeline.AddRootSignatureAssociation(shadowHitSignature.Get(), { L"ShadowHitGroup" });
	pipeline.AddRootSignatureAssociation(missSignature.Get(), { L"Miss",L"ShadowMiss" });


	pipeline.AddRootSignatureAssociation(hitSignature.Get(), { L"HitGroup",L"PlaneHitGroup" });









	pipeline.SetMaxPayloadSize(7 * sizeof(float)); /// RGB + DISTANCE + canReflect + maxdepth + currentdepth
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // Barycentric coordinates


	// Raytracing can shoot rays from existing points leading to nested TraceRay function calls.

	// Trace Depth: 1 means only primary/initial rays are taken into account
	// Recursion should be kept to a minimum

	//path tracing algorithms can be flattened into a simple loop in ray generation

	pipeline.SetMaxRecursionDepth(4);

	rtStateObject = pipeline.Generate();

	ThrowIfFailed(rtStateObject->QueryInterface(IID_PPV_ARGS(&rtStateObjectProps)), L"Unable to create raytracing pipeline");
}

void RenderApplication::CreateRaytracingOutputBuffer()
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
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&outputResource)), L"Unable to create image output buffer");

}

AccelerationStructureBuffers RenderApplication::CreateBottomLevelAS(std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers, std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers)
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
//	buffers.pScratch=

	buffers.pResult = nv_helpers_dx12::CreateBuffer(device, resultSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);


	// Finally we can build the acceleration structure NOTE: THIS CALL INTEGRATES A RESOURCE BARRIER ON THE GENERATED AS
	// This is done so that the BLAS can be used to compute a TLAS after this method.

	bottomLevelAS.Generate(commandList, buffers.pScratch.Get(), buffers.pResult.Get(), false, nullptr);


	return buffers;
}

void RenderApplication::CreateTopLevelAS(const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly)
{
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



void RenderApplication::CreateNamedTopLevelAS(const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly, std::string name)
{
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

		scenes[name].pScratch = nv_helpers_dx12::CreateBuffer(device, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nv_helpers_dx12::kDefaultHeapProps);

		scenes[name].pResult = nv_helpers_dx12::CreateBuffer(device, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);

		scenes[name].pInstanceDesc = nv_helpers_dx12::CreateBuffer(device, instanceDescSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	}

	topLevelASGenerator.Generate(commandList,
		scenes[name].pScratch.Get(),
		scenes[name].pResult.Get(),
		scenes[name].pInstanceDesc.Get(),
		updateOnly,
		scenes[name].pResult.Get());


}

void RenderApplication::CreateAccelerationStructures()
{
	HRESULT hr;

	AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS({ { modelResourceHandler->getVertexBuffer("TEAPOT"),modelResourceHandler->getModelVertNumber("TEAPOT")}}, {{modelResourceHandler->getIndexBuffer("TEAPOT"),modelResourceHandler->getModelIndexNumber("TEAPOT")}});


//	AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS({ { vertexBuffer, teapotVertNumber } }, { {indexBuffer, teapotIndexNumber} });

	AccelerationStructureBuffers planeBottomLevelBuffer = CreateBottomLevelAS({ {planeBuffer.Get(), 6} }, {});

	instances = { {bottomLevelBuffers.pResult, DirectX::XMMatrixIdentity()},
		{bottomLevelBuffers.pResult, DirectX::XMMatrixTranslation(-1.0,0,0) },
		{bottomLevelBuffers.pResult, DirectX::XMMatrixTranslation(1.0,0,0)},
		{planeBottomLevelBuffer.pResult, DirectX::XMMatrixTranslation(0,0,0) * DirectX::XMMatrixScaling(100,100,100)} }  ;

	CreateTopLevelAS(instances, false);



//	AccelerationStructureBuffers bottomLevelBuffersSceneTwo = CreateBottomLevelAS({ { modelResourceHandler->getVertexBuffer("CUBE"),modelResourceHandler->getModelVertNumber("CUBE")} }, 
//		{ {modelResourceHandler->getIndexBuffer("TEAPOT"),modelResourceHandler->getModelIndexNumber("CUBE")} });


//	scenetwo_instances= { {bottomLevelBuffersSceneTwo.pResult, DirectX::XMMatrixIdentity()},
//	{bottomLevelBuffersSceneTwo.pResult, DirectX::XMMatrixTranslation(-1.0,0,0) },
//	{bottomLevelBuffersSceneTwo.pResult, DirectX::XMMatrixTranslation(1.0,0,0)}, };
//	CreateNamedTopLevelAS(scenetwo_instances, false, "TEST");
	
	//CreateScene2(scenetwo_instances, false,"TEST");
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

ComPtr<ID3D12RootSignature> RenderApplication::CreateRayGenSignature()
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
		2,//camera
		}
		});


	return rsc.Generate(device, true);
}

ComPtr<ID3D12RootSignature> RenderApplication::CreateHitSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;

	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0/*t0*/);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1/*t1*/);

	rsc.AddHeapRangesParameter(
		{
			{
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
				3 /*4th heap slot*/
			} /* Per Instance Data*/
			,
			{
				4/*t4*/,
				1,
				0,
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				6, //TODO: Replace with proper heap slot for albedo texture
			},
			{ //albedo
			5,
			1,
			0,
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				7 /* heap slot 8*/
			},
			
			{ //metal
				6,
				1,
				0,
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				8
			},
			{ //roughness
				7,
				1,
				0,
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				9
			}, 
					{ //normal
				8,
				1,
				0,
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				10
			},
		}
	);

	rsc.AddHeapRangesParameter(
		{ { 0,1,0,
	D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
	0} });


	return rsc.Generate(device, true);
}

ComPtr<ID3D12RootSignature> RenderApplication::CreateMissSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;

	//rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0);

	rsc.AddHeapRangesParameter({
		{
			0,
			1,
			0,
			D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
			4 //5th heap slot
		},
	{
		0,
		1,
		0,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		5//6th heap slot
	}
	});
//	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV,0);
		//TODO: Finish binding data to miss shader
	return rsc.Generate(device, true);
}

void RenderApplication::CreateEnvmapResourceHeap()
{

//	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
//	desc.NumDescriptors = 1;
//	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//		ID3D12DescriptorHeap* pHeap;
//	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&envmapHeap)), L"Failed to create descriptor heap in DXRHelper L162");
//	envmapHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	
//	D3D12_CPU_DESCRIPTOR_HANDLE envHeapDescHandle = envmapHeap->GetCPUDescriptorHandleForHeapStart();

		//	D3D12_CONSTANT_BUFFER_VIEW_DESC bgDesc = {};
	//	bgDesc.BufferLocation = backgroundColor->GetGPUVirtualAddress();
	//	bgDesc.SizeInBytes = sizeof(DirectX::XMVECTOR);

	//	device->CreateConstantBufferView(&bgDesc, envHeapDescHandle);
//


}

void RenderApplication::CreateShaderResourceheap()
{

	 tempSamplerHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);


	D3D12_CPU_DESCRIPTOR_HANDLE tsHandle = tempSamplerHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_SAMPLER_DESC samplerDesc{};

	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 0;
	device->CreateSampler(&samplerDesc, tsHandle);



	srvUAVHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 11, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

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



	D3D12_CONSTANT_BUFFER_VIEW_DESC bgDesc = {};

	bgDesc.BufferLocation = backgroundColor->GetGPUVirtualAddress();
	bgDesc.SizeInBytes = sizeof(DirectX::XMVECTOR) * 16;
	device->CreateConstantBufferView(&bgDesc, srvHandle);

	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	D3D12_SHADER_RESOURCE_VIEW_DESC envmap_srvDesc = {};
	envmap_srvDesc.Format = image->GetMetadata().format;
	envmap_srvDesc.Texture2D.MipLevels = image->GetMetadata().mipLevels;


	envmap_srvDesc.Texture2D.ResourceMinLODClamp = 0;
	envmap_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	envmap_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(env_texture.Get(), &envmap_srvDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC testTextureDesc = {};

	testTextureDesc.Format = textureMetaData["TEST"].format;
	testTextureDesc.Texture2D.MipLevels = textureMetaData["TEST"].mipLevels;

	testTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	testTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	testTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["TEST"].Get(), &testTextureDesc, srvHandle);

	
	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC albedoTextureDesc = {};

	albedoTextureDesc.Format = textureMetaData["ALBEDO"].format;
	albedoTextureDesc.Texture2D.MipLevels = textureMetaData["ALBEDO"].mipLevels;

	albedoTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	albedoTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	albedoTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["ALBEDO"].Get(), &albedoTextureDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC metalTextureDesc = {};

	metalTextureDesc.Format = textureMetaData["METAL"].format;
	metalTextureDesc.Texture2D.MipLevels = textureMetaData["METAL"].mipLevels;

	metalTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	metalTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	metalTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["METAL"].Get(), &metalTextureDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC roughnessTextureDesc = {};

	roughnessTextureDesc.Format = textureMetaData["ROUGHNESS"].format;
	roughnessTextureDesc.Texture2D.MipLevels = textureMetaData["ROUGHNESS"].mipLevels;

	 roughnessTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	roughnessTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	roughnessTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["ROUGHNESS"].Get(), &roughnessTextureDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	D3D12_SHADER_RESOURCE_VIEW_DESC normalTextureDesc = {};
	normalTextureDesc.Format = textureMetaData["NORMAL"].format;
	normalTextureDesc.Texture2D.MipLevels = textureMetaData["NORMAL"].mipLevels;

	normalTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	normalTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	normalTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["NORMAL"].Get(), &normalTextureDesc, srvHandle);


//	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

//	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	
}

void RenderApplication::CreateShaderBindingTable()
{
	sbtHelper.Reset();

	D3D12_GPU_DESCRIPTOR_HANDLE srvUAVHeapHandle =
		srvUAVHeap->GetGPUDescriptorHandleForHeapStart();


//	D3D12_GPU_DESCRIPTOR_HANDLE envHeap = envmapHeap->GetGPUDescriptorHandleForHeapStart();
	auto heapPointer = reinterpret_cast<UINT64*>(srvUAVHeapHandle.ptr);
	auto samplerHeapPointer = reinterpret_cast<UINT64*>(tempSamplerHeap->GetGPUDescriptorHandleForHeapStart().ptr);

//	auto envheapPointer = reinterpret_cast<UINT64*>(envHeap.ptr);

	sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });

	// Miss and hit shaders do not access external resources, they communicate results through the ray payload instead.
	sbtHelper.AddMissProgram(L"Miss", { heapPointer});
	sbtHelper.AddMissProgram(L"ShadowMiss", {});





	/// <summary>
	/// TODO: Create a ResourceManager that handles GPU Resources, then use identifiers to hash into a container`and get the required resources for each model
	/// </summary>
	for (int i = 0; i < instances.size() - 1; i++)
	{
		sbtHelper.AddHitGroup(L"HitGroup", { (void*)vertexBuffer->GetGPUVirtualAddress(),(void*)indexBuffer->GetGPUVirtualAddress(),heapPointer,samplerHeapPointer/*(void*)materialproperties->GetGPUVA*/});
		sbtHelper.AddHitGroup(L"ShadowHitGroup", {});
	}

	sbtHelper.AddHitGroup(L"PlaneHitGroup", { (void*)planeBuffer->GetGPUVirtualAddress(),});
	sbtHelper.AddHitGroup(L"ShadowHitGroup", {});
	// add triangle shader
//	sbtHelper.AddHitGroup(L"HitGroup", {(void*)(vertexBuffer->GetGPUVirtualAddress()),(void*)(indexBuffer->GetGPUVirtualAddress()),(void*)globalConstBuffer->GetGPUVirtualAddress()});





	uint32_t sbtSize = sbtHelper.ComputeSBTSize();




	sbtStorage = nv_helpers_dx12::CreateBuffer(device, sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	if (!sbtStorage)
	{
		throw std::logic_error("Could not allocate the shader binding table");
	}

	sbtHelper.Generate(sbtStorage.Get(), rtStateObjectProps.Get());
}

void RenderApplication::CreateScene2()
{

	SceneTwoHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 11, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = SceneTwoHeap->GetCPUDescriptorHandleForHeapStart();

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
	srvDesc.RaytracingAccelerationStructure.Location =scenes["TEST"].pResult->GetGPUVirtualAddress();

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



	D3D12_CONSTANT_BUFFER_VIEW_DESC bgDesc = {};

	bgDesc.BufferLocation = backgroundColor->GetGPUVirtualAddress();
	bgDesc.SizeInBytes = sizeof(DirectX::XMVECTOR) * 16;
	device->CreateConstantBufferView(&bgDesc, srvHandle);

	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	D3D12_SHADER_RESOURCE_VIEW_DESC envmap_srvDesc = {};
	envmap_srvDesc.Format = image->GetMetadata().format;
	envmap_srvDesc.Texture2D.MipLevels = image->GetMetadata().mipLevels;


	envmap_srvDesc.Texture2D.ResourceMinLODClamp = 0;
	envmap_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	envmap_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(env_texture.Get(), &envmap_srvDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC testTextureDesc = {};

	testTextureDesc.Format = textureMetaData["TEST"].format;
	testTextureDesc.Texture2D.MipLevels = textureMetaData["TEST"].mipLevels;

	testTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	testTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	testTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["TEST"].Get(), &testTextureDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC albedoTextureDesc = {};

	albedoTextureDesc.Format = textureMetaData["ALBEDO"].format;
	albedoTextureDesc.Texture2D.MipLevels = textureMetaData["ALBEDO"].mipLevels;

	albedoTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	albedoTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	albedoTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["ALBEDO"].Get(), &albedoTextureDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC metalTextureDesc = {};

	metalTextureDesc.Format = textureMetaData["METAL"].format;
	metalTextureDesc.Texture2D.MipLevels = textureMetaData["METAL"].mipLevels;

	metalTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	metalTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	metalTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["METAL"].Get(), &metalTextureDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC roughnessTextureDesc = {};

	roughnessTextureDesc.Format = textureMetaData["ROUGHNESS"].format;
	roughnessTextureDesc.Texture2D.MipLevels = textureMetaData["ROUGHNESS"].mipLevels;

	roughnessTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	roughnessTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	roughnessTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["ROUGHNESS"].Get(), &roughnessTextureDesc, srvHandle);


	srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	D3D12_SHADER_RESOURCE_VIEW_DESC normalTextureDesc = {};
	normalTextureDesc.Format = textureMetaData["NORMAL"].format;
	normalTextureDesc.Texture2D.MipLevels = textureMetaData["NORMAL"].mipLevels;

	normalTextureDesc.Texture2D.ResourceMinLODClamp = 0;
	normalTextureDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	normalTextureDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	device->CreateShaderResourceView(textures["NORMAL"].Get(), &normalTextureDesc, srvHandle);

}
