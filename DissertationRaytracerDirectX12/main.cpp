#include "RenderApplication.h"

/*
#include <shlobj.h>
#include <strsafe.h>

static std::wstring GetLatestWinPixGpuCapturerPath()
{
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    std::wstring pixSearchPath = programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

    WIN32_FIND_DATA findData;
    bool foundPixInstallation = false;
    wchar_t newestVersionFound[MAX_PATH];

    HANDLE hFind = FindFirstFile(pixSearchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
                (findData.cFileName[0] != '.'))
            {
                if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
                {
                    foundPixInstallation = true;
                    StringCchCopy(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
                }
            }
        } while (FindNextFile(hFind, &findData) != 0);
    }

    FindClose(hFind);

    if (!foundPixInstallation)
    {
        // TODO: Error, no PIX installation found
    }

    wchar_t output[MAX_PATH];
    StringCchCopy(output, pixSearchPath.length(), pixSearchPath.data());
    StringCchCat(output, MAX_PATH, &newestVersionFound[0]);
    StringCchCat(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

    return &output[0];
}*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
   //  if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
//    {
  //      LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
  //  }

	/*
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


	return 0;*/

	RenderApplication app;
	app.setup(hInstance, nCmdShow, 1920, 1080, true);

	return 0;
}


