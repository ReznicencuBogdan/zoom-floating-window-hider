#pragma once
#include <Windows.h>

#define MAX_EXPANDED_PATH 512
LPCWSTR pswLibraryPath = L"%appdata%\\ZoomFloatingWindowHider\\zfloatlib.dll";

DWORD GetExpandedEnvironmentStringsW(const wchar_t* lpSource, wchar_t* lpDestination)
{
	DWORD size = ExpandEnvironmentStringsW(lpSource, NULL, 0);
	DWORD ret = ExpandEnvironmentStringsW(lpSource, lpDestination, size);

	if (ret != size)
	{
		return 0;
	}

	return size;
}

BOOL RemoteLoadLibrary(HANDLE hProcess, const wchar_t* pwsLibPath)
{
	LPVOID lpLoadLibraryW;
	LPVOID lpRemoteAddress;
	HMODULE hModKernel32;
	SIZE_T nLength;

	ZeroMemory(&hModKernel32, sizeof(hModKernel32));
	nLength = wcslen(pwsLibPath) * sizeof(wchar_t);

	hModKernel32 = GetModuleHandle(TEXT("Kernel32.dll"));

	if (!hModKernel32)
	{
		return FALSE;
	}

	lpLoadLibraryW = GetProcAddress(hModKernel32, "LoadLibraryW");

	if (!lpLoadLibraryW)
	{
		return FALSE;
	}

	// Create enough space to hold library path name
	lpRemoteAddress = VirtualAllocEx(hProcess, NULL, nLength + 1, MEM_COMMIT, PAGE_READWRITE);

	if (!lpRemoteAddress)
	{
		return FALSE;
	}

	// Write library path name at alloted space
	if (!WriteProcessMemory(hProcess, lpRemoteAddress, pwsLibPath, nLength, NULL))
	{
		// free allocated memory
		VirtualFreeEx(hProcess, lpRemoteAddress, 0, MEM_RELEASE);

		return FALSE;
	}


	HANDLE hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)lpLoadLibraryW, lpRemoteAddress, NULL, NULL);

	if (!hThread)
	{
	}
	else
	{
		WaitForSingleObject(hThread, 4000);
	}

	// Free allocated memory
	VirtualFreeEx(hProcess, lpRemoteAddress, 0, MEM_RELEASE);

	return true;
}