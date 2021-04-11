#pragma once
#include <functional>
#include <thread>

#include <dwmapi.h>
#pragma comment (lib, "dwmapi.lib")

#include <d3d9.h>
#pragma comment (lib, "d3d9.lib")

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "XorStr.h"

namespace D3DOverlay
{
	class COverlay
	{
	private:
		static std::function<void(int, int)> m_UserRender;
		static HWND m_Window;
		static HWND m_TargetWindow;
		static WNDCLASSEX m_WindowClass;
		static int m_WindowWidth;
		static int m_WindowHeight;
		static RECT m_OldRect;

		static LPDIRECT3D9 m_D3D;
		static LPDIRECT3DDEVICE9 m_D3Device;
		static D3DPRESENT_PARAMETERS m_D3Parameters;

		static bool CreateOverlay(LPCSTR WindowClassName, LPCSTR WindowName);
		static bool CreateDeviceD3D();
		static LRESULT __stdcall WndProc(HWND aHwnd, UINT aMsg, WPARAM aWParam, LPARAM aLParam);

	public:
		static bool Init(const HWND TargetWindow, LPCSTR WindowClassName, LPCSTR WindowName);
		static bool Update(bool MenuOpen);
		static void SetUserRender(const std::function<void(int, int)> UserRender);
		static void CleanupOverlay();
	};
}