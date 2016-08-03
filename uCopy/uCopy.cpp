// uCopy.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "uCopy.h"

#include <dbt.h>
#include <strsafe.h>
#include "Shlwapi.h"
#include "Shlobj.h"
#include <shellapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
LPWSTR *szArgList;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void DeviceChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
char FirstDriveFromMask(ULONG unitmask);
void FindFilesRecursively(LPCTSTR lpFolder, LPCTSTR lpTargetFolder, LPCTSTR lpFilePattern);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	int argCount;
	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (szArgList == NULL || argCount != 3)
	{
		MessageBox(NULL, _T("Command Arguments: uCopy \"copy to folder\" \"filter\"\nExit: Ctrl+Shift+X"), _T("uCopy"), MB_OK);
		return 10;
	}

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_UCOPY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UCOPY));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	//ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	RegisterHotKey(hWnd, 100, MOD_CONTROL | MOD_SHIFT, 0x58); // Ctrl+Shift+X
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_HOTKEY:
		PostQuitMessage(0);
		break;
	case WM_DEVICECHANGE:
		DeviceChange(hWnd, wParam, lParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void DeviceChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	TCHAR szMsg[80];
	if (DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam) {
		PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
		PDEV_BROADCAST_VOLUME pDevVolume;
		switch (pHdr->dbch_devicetype) {
		case DBT_DEVTYP_VOLUME:
			pDevVolume = (PDEV_BROADCAST_VOLUME)pHdr;
			if (DBT_DEVICEARRIVAL == wParam) {
				StringCchPrintf(szMsg, sizeof(szMsg) / sizeof(szMsg[0]),
					TEXT("%c:"),
					FirstDriveFromMask(pDevVolume->dbcv_unitmask));
				FindFilesRecursively(szMsg, szArgList[1], szArgList[2]);
			}
			break;
		}
	}
}

char FirstDriveFromMask(ULONG unitmask)
{
	char i;
	for (i = 0; i < 26; ++i)
	{
		if (unitmask & 0x1)
			break;
		unitmask = unitmask >> 1;
	}
	return(i + 'A');
}

void FindFilesRecursively(LPCTSTR lpFolder, LPCTSTR lpTargetFolder, LPCTSTR lpFilePattern)
{
	TCHAR szFullPattern[MAX_PATH];
	TCHAR szTarget[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile;
	PathCombine(szFullPattern, lpFolder, _T("*"));
	hFindFile = FindFirstFile(szFullPattern, &FindFileData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (FindFileData.cFileName[0] == '.') continue;
				PathCombine(szFullPattern, lpFolder, FindFileData.cFileName);
				PathCombine(szTarget, lpTargetFolder, FindFileData.cFileName);
				FindFilesRecursively(szFullPattern, szTarget, lpFilePattern);

			}
		} while (FindNextFile(hFindFile, &FindFileData));
		FindClose(hFindFile);
	}

	PathCombine(szFullPattern, lpFolder, lpFilePattern);
	hFindFile = FindFirstFile(szFullPattern, &FindFileData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				PathCombine(szFullPattern, lpFolder, FindFileData.cFileName);
				SHCreateDirectoryEx(NULL, lpTargetFolder, NULL);
				PathCombine(szTarget, lpTargetFolder, FindFileData.cFileName);
				CopyFile(szFullPattern, szTarget, 0);
				//_tprintf_s(_T("%s\n"), szFullPattern);
			}
		} while (FindNextFile(hFindFile, &FindFileData));
		FindClose(hFindFile);
	}
}