#include "stdafx.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{




	bool FullScreen =false;

	if (!InitializeWindow(hInstance, nCmdShow, Width, Height,FullScreen))
	{
		MessageBox(0, L"Window Initializaiton - Failed", L"Error", MB_OK);
		return 0;
	}
	
	mainloop();
	

	return 0;
}


