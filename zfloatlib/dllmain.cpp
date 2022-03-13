// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "procinj.h"
#include <string>

#define IS_ATOM(x) (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))


typedef HWND(WINAPI* CREATEWINDOWEXW)(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCWSTR lpClassName,
	_In_opt_ LPCWSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam);

typedef BOOL(WINAPI* CREATEPROCESSW)(
	_In_opt_ LPCWSTR lpApplicationName,
	_Inout_opt_ LPWSTR lpCommandLine,
	_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
	_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	_In_ BOOL bInheritHandles,
	_In_ DWORD dwCreationFlags,
	_In_opt_ LPVOID lpEnvironment,
	_In_opt_ LPCWSTR lpCurrentDirectory,
	_In_ LPSTARTUPINFOW lpStartupInfo,
	_Out_ LPPROCESS_INFORMATION lpProcessInformation
	);

typedef ATOM(WINAPI* REGISTERCLASSEXW)(_In_ CONST WNDCLASSEXW*);

CREATEWINDOWEXW fpCreateWindowExW = NULL;
CREATEPROCESSW fpCreateProcess = NULL;
REGISTERCLASSEXW fpRegisterClassExW = NULL;

BOOL WINAPI DetourCreateProcessW(
	_In_opt_ LPCWSTR lpApplicationName,
	_Inout_opt_ LPWSTR lpCommandLine,
	_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
	_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	_In_ BOOL bInheritHandles,
	_In_ DWORD dwCreationFlags,
	_In_opt_ LPVOID lpEnvironment,
	_In_opt_ LPCWSTR lpCurrentDirectory,
	_In_ LPSTARTUPINFOW lpStartupInfo,
	_Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
	WCHAR pswExpandedLibPth[MAX_EXPANDED_PATH];
	ZeroMemory(pswExpandedLibPth, sizeof(pswExpandedLibPth));

	if (GetExpandedEnvironmentStringsW(pswLibraryPath, pswExpandedLibPth) > 0)
	{
		dwCreationFlags |= CREATE_SUSPENDED;
	}

	BOOL result = fpCreateProcess(
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation
	);

	if (result && ((dwCreationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED))
	{
		RemoteLoadLibrary(lpProcessInformation->hProcess, pswExpandedLibPth);
		ResumeThread(lpProcessInformation->hThread);
	}

	return result;
}


BOOL GetIsClassNameAllowedForRegistration(LPCWSTR lpClassName)
{
	if (!IS_ATOM(lpClassName))
	{
		BOOL flagClassCaught = FALSE ||
			(wcsstr(lpClassName, L"zFocusNotificationWndClass") != NULL) ||
			(wcsstr(lpClassName, L"ZPFloatToolbarClass") != NULL) ||
			(wcsstr(lpClassName, L"zFloatWidgetNotifiWndClass") != NULL) ||
			(wcsstr(lpClassName, L"zMeetingNotificationWndClass") != NULL);

		if (flagClassCaught)
		{
			return FALSE;
		}
	}

	return TRUE;
}

HWND WINAPI DetourCreateWindowExW(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCWSTR lpClassName,
	_In_opt_ LPCWSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam)
{
	if (lpClassName)
	{
		BOOL flagValidated = GetIsClassNameAllowedForRegistration(lpClassName);

		if (flagValidated == FALSE)
		{
			return NULL;
		}
	}

	return fpCreateWindowExW(
		dwExStyle,
		lpClassName,
		lpWindowName,
		dwStyle,
		X,
		Y,
		nWidth,
		nHeight,
		hWndParent,
		hMenu,
		hInstance,
		lpParam);
}

ATOM WINAPI DetourRegisterClassExW(_In_ CONST WNDCLASSEXW* lpWndClassEx)
{
	if (lpWndClassEx->lpszClassName != NULL)
	{
		BOOL flagValidated = GetIsClassNameAllowedForRegistration(lpWndClassEx->lpszClassName);

		if (flagValidated == FALSE)
		{
			return NULL;
		}
	}

	return fpRegisterClassExW(lpWndClassEx);
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);

		// Initialize MinHook.
		if (MH_Initialize() != MH_OK)
		{
			return 1;
		}

		// Create a hook
		if (MH_CreateHook(&CreateWindowExW, &DetourCreateWindowExW, reinterpret_cast<LPVOID*>(&fpCreateWindowExW)) != MH_OK)
		{
			return 1;
		}
		if (MH_EnableHook(&CreateWindowExW) != MH_OK)
		{
			return 1;
		}
		//
		if (MH_CreateHook(&CreateProcessW, &DetourCreateProcessW, reinterpret_cast<LPVOID*>(&fpCreateProcess)) != MH_OK)
		{
			return 1;
		}
		if (MH_EnableHook(&CreateProcessW) != MH_OK)
		{
			return 1;
		}
		//
		if (MH_CreateHook(&RegisterClassExW, &DetourRegisterClassExW, reinterpret_cast<LPVOID*>(&fpRegisterClassExW)) != MH_OK)
		{
			return 1;
		}
		if (MH_EnableHook(&RegisterClassExW) != MH_OK)
		{
			return 1;
		}

		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
	{
		MH_DisableHook(&CreateProcessW);
		MH_RemoveHook(&CreateProcessW);

		MH_DisableHook(&CreateWindowExW);
		MH_RemoveHook(&CreateWindowExW);

		MH_DisableHook(&RegisterClassExW);
		MH_RemoveHook(&RegisterClassExW);

		MH_Uninitialize();
		break;
	}
	}
	return TRUE;
}

