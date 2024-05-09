#pragma once
#include "stdafx.h"
#include "tiny_obj_loader.h"
#include "Imgui/imgui.h"
#include "StepTimer.h"
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
	Vertex(float x, float y, float z, float r, float g, float b, float a, float u, float v) : pos(x, y, z), color(r, g, b, a), uv{ u, v } {}
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 uv;
};

struct Model
{
	ComPtr<ID3D12Resource> vertexBuffer;
	UINT64 vertex_count;
	ComPtr<ID3D12Resource> indexBuffer;
	UINT64 index_count;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
};


class ModelResourceHandler
{

	tinyobj::ObjReaderConfig reader_config;
	tinyobj::ObjReader reader;
	std::map<std::string, std::unique_ptr<Model>> modelMap;

public:
	D3D12_VERTEX_BUFFER_VIEW& getModelVertexBufferView(std::string name)
	{
		return modelMap.at(name)->vertex_buffer_view;
	}

	D3D12_INDEX_BUFFER_VIEW& getModelIndexView(std::string name)
	{
		return modelMap.at(name)->index_buffer_view;
	}

	ComPtr<ID3D12Resource>& getVertexBuffer(std::string name)
	{
		return modelMap[name]->vertexBuffer;
	}
	ComPtr<ID3D12Resource>& getIndexBuffer(std::string name)
	{
		return modelMap[name]->indexBuffer;
	}

	UINT64 getModelVertNumber(std::string name)
	{
		return modelMap[name]->vertex_count;
	}
	UINT64 getModelIndexNumber(std::string name)
	{
		return modelMap[name]->index_count;
	}

	


	ModelResourceHandler()
	{
		modelMap = std::map < std::string, std::unique_ptr<Model>>();
	}

	bool loadFromObj(std::string filename, ID3D12Device5* device,ID3D12GraphicsCommandList* commandlist, std::string name)
	{
		modelMap[name] = std::make_unique<Model>();


		if (!reader.ParseFromFile(filename, reader_config))
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


		std::vector<UINT> indices;
		for (auto index : importIndices)
		{
			indices.push_back(static_cast<UINT>(index.vertex_index));



		}
		std::vector<float> uvs;

		attribs.texcoords;

		modelMap[name]->index_count = indices.size();

		std::vector<Vertex> vertices;

		if (!shapes.empty())
		{
			std::printf("Successfully imported model");
		}

		//float prescale = 0.1;

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

			tempvert.uv.x = attribs.texcoords[i * 2 + 0];
			tempvert.uv.y = attribs.texcoords[i * 2 + 1];


			vertices.push_back(tempvert);

		}
		modelMap[name]->vertex_count = vertices.size();

		if (!vertices.empty())
		{
			printf("Success");
		}


		
		int vBufferSize = modelMap[name]->vertex_count * sizeof(Vertex);

		const UINT iBufferSize = static_cast<UINT>(modelMap[name]->index_count) * sizeof(UINT);

		auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto resource_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);



		device->CreateCommittedResource(
			&heap_props,
			D3D12_HEAP_FLAG_NONE,
			&resource_buffer_desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&modelMap[name]->vertexBuffer));

		modelMap[name]->vertexBuffer->SetName(L"vbuffer");




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

		vertexData.pData = reinterpret_cast<BYTE*>(vertices.data());
		vertexData.RowPitch = vBufferSize;
		vertexData.SlicePitch = vBufferSize;



		//Upload heap from CPU to GPU

		UpdateSubresources(commandlist,
			modelMap[name]->vertexBuffer.Get(),
			vBufferUploadHeap,
			0,
			0,
			1,
			&vertexData);

		//FIX LVALUE
		auto rbtemp = CD3DX12_RESOURCE_BARRIER::Transition(modelMap[name]->vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);





		CD3DX12_HEAP_PROPERTIES iBufferHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(iBufferSize);
		
		device->CreateCommittedResource(
			&iBufferHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferResource,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&modelMap[name]->indexBuffer)
		);

		UINT8* pIndexDataBegin;
		D3D12_RANGE readRange = { 0,0 };

		modelMap[name]->indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
		memcpy(pIndexDataBegin, indices.data(), iBufferSize);
		modelMap[name]->indexBuffer->Unmap(0, nullptr);

		modelMap[name]->index_buffer_view.BufferLocation = modelMap[name]->indexBuffer->GetGPUVirtualAddress();
		modelMap[name]->index_buffer_view.Format = DXGI_FORMAT_R32_UINT,
		modelMap[name]->index_buffer_view.SizeInBytes = iBufferSize;

		return true;
	}



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

	std::string inputfile = "teapotuv.obj";
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

	DX::StepTimer timer;

	float temprot =0.0;


	void CreateScene2();
	

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

	std::map < std::string, DirectX::TexMetadata > textureMetaData;
	std::unique_ptr<DirectX::ScratchImage> image;
	ComPtr<ID3D12Resource> uploadHeap;

	void LoadTextureFromFile(const wchar_t* filename,std::string name);

	DirectX::TexMetadata tempdata;

	void CreatePerInstancePropertiesBuffer();
	void UpdatePerInstancePropertiesBuffer();



	void CreateCameraBuffer();
	void UpdateCameraBuffer();

	std::map < std::string, ComPtr<ID3D12Resource> >tempHeaps;
	
	void CreatePlaneVB(); 

	/*
	 *
	 */
	void CreateNamedTopLevelAS(const std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly, std::string name);
	std::unique_ptr<ModelResourceHandler>  modelResourceHandler = std::make_unique<ModelResourceHandler>();

	bool InitD3D();
	bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool fullscreen);
	void mainloop();
	void UpdatePipeline();

	void WaitForPreviousFrame();

	void Cleanup();

	void Tick();
	void update(DX::StepTimer const& step_timer);
	void Render();


	//void OnKeyUp(UINT8 key);

	std::map<std::string, ComPtr<ID3D12Resource>> textures;

	double currentTime = 0.0;

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


	std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> scenetwo_instances;


	ComPtr<ID3D12Resource> cameraBuffer;
	ComPtr<ID3D12DescriptorHeap> constHeap;

	ComPtr<ID3D12DescriptorHeap> tempSamplerHeap;

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


	ComPtr<ID3D12DescriptorHeap> SceneTwoHeap;

	CameraController cameraController;

	ComPtr<IDxcBlob> rayGenLibrary;
	ComPtr<IDxcBlob> hitLibrary;
	ComPtr<IDxcBlob> missLibrary;
	ComPtr<IDxcBlob> shadowLibrary;


	ComPtr<ID3D12RootSignature> rayGenSignature;
	ComPtr<ID3D12RootSignature> hitSignature;
	ComPtr<ID3D12RootSignature> missSignature;
	ComPtr<ID3D12RootSignature> shadowHitSignature;


	std::map<std::string, AccelerationStructureBuffers> scenes;


	ComPtr<ID3D12StateObject> rtStateObject;

	ComPtr<ID3D12StateObjectProperties> rtStateObjectProps;

	AccelerationStructureBuffers SceneTwoASBuffers;


	ComPtr<ID3D12Resource> planeBuffer;

	ComPtr<ID3D12Resource> indexBuffer;
	ComPtr<ID3D12Resource> backgroundColor;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	D3D12_VERTEX_BUFFER_VIEW planeBufferview;
	bool m_raster = true;



	//TODO: Remove temp rotspeed variable
	float rotspeed = 0.0;
	std::uint32_t game_time;

	ComPtr<ID3D12Resource> perInstancePropertiesBuffer;
	DirectX::XMVECTOR bgcol;
};

