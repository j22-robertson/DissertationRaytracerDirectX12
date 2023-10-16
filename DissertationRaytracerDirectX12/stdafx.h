#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"



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

int Width = 800;
int Height = 600;


bool FullScreen = false;

bool Running = true;

const int frameBufferCount = 3; // buffer num, 2 = double buffering 3 = triple


ID3D12Device* device;

IDXGISwapChain3* swapChain;

ID3D12CommandQueue* commandQueue;

ID3D12DescriptorHeap* rtvDescriptorHeap; // Holds resources e.g. render targets

ID3D12Resource* renderTargets[frameBufferCount];

ID3D12CommandAllocator* commandAllocator[frameBufferCount];

ID3D12GraphicsCommandList* commandList;

ID3D12Fence* fence[frameBufferCount];

HANDLE fenceEvent;

UINT64 fenceValue[frameBufferCount];

int frameIndex;

int rtvDescriptorSize;

bool InitD3D();

void Update();

void UpdatePipeline();

void Render();


void WaitForPreviousFrame();
void Cleanup();


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			if (MessageBox(0, L"Are you sure you wnat to exit?", L"Confirm, do you want to exit program?", MB_YESNO | MB_ICONQUESTION) == IDYES)
				DestroyWindow(hWnd);
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	}
	return DefWindowProc(hWnd, msg, wParam, lParam);

}






