// start.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "procinj.h"
#include <iostream>

LPCWSTR pswZoomPath = L"%appdata%\\Zoom\\bin\\Zoom.exe";


int main()
{
	WCHAR pswExpandedZoomPth[MAX_EXPANDED_PATH];
	WCHAR pswExpandedLibPth[MAX_EXPANDED_PATH];
	ZeroMemory(pswExpandedZoomPth, sizeof(pswExpandedZoomPth));
	ZeroMemory(pswExpandedLibPth, sizeof(pswExpandedLibPth));

	if (GetExpandedEnvironmentStringsW(pswZoomPath, pswExpandedZoomPth) == 0)
	{
		std::wcout << L"Not enough memory to store expanded zoom path";
		return 0;
	}
	if (GetExpandedEnvironmentStringsW(pswLibraryPath, pswExpandedLibPth) == 0)
	{
		std::wcout << L"Not enough memory to store expanded library path";
		return 0;
	}

	PROCESS_INFORMATION processInformation;
	STARTUPINFO startupInfo;
	ZeroMemory(&processInformation, sizeof(processInformation));
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	if (!CreateProcess(
		pswExpandedZoomPth,
		NULL,
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED,
		NULL,
		NULL,
		&startupInfo,
		&processInformation))
	{
		std::wcout << L"Failed creating process.";
		return 1;
	}

	std::wcout << "Process created.";

	if (RemoteLoadLibrary(processInformation.hProcess, pswExpandedLibPth))
	{
		std::wcout << L"Process injected.";
		ResumeThread(processInformation.hThread);
	}
	else
	{
		std::wcout << L"Failed injecting process.";
	}

	CloseHandle(processInformation.hProcess);
	CloseHandle(processInformation.hThread);
}