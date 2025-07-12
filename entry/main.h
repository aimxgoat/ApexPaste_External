#pragma once
#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <Psapi.h>
#include <dwmapi.h>
#include <d3d9.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"
#include "kernel/driver.h"
#pragma comment(lib, "dwmapi.lib")

using namespace std;
namespace fs = std::filesystem;

int Width = GetSystemMetrics(SM_CXSCREEN);
int Height = GetSystemMetrics(SM_CYSCREEN);


IDirect3DTexture9* sillycar = nullptr;

struct CurrentProcess {
	DWORD ID;
	HANDLE Handle;
	HWND Hwnd;
	WNDPROC WndProc;
	int WindowWidth;
	int WindowHeight;
	int WindowLeft;
	int WindowRight;
	int WindowTop;
	int WindowBottom;
	LPCSTR Title;
	LPCSTR ClassName;
	LPCSTR Path;
}Process;

struct OverlayWindow {
	WNDCLASSEX WindowClass;
	HWND Hwnd;
	LPCSTR Name;
}Overlay;

struct DirectX9Interface {
	IDirect3D9Ex* IDirect3D9 = NULL;
	IDirect3DDevice9Ex* pDevice = NULL;
	D3DPRESENT_PARAMETERS pParameters = { NULL };
	MARGINS Margin = { -1 };
	MSG Message = { NULL };
}DirectX9;

#include "overlay/overlay.hpp"
#include "engine/cheat/loop.hpp"