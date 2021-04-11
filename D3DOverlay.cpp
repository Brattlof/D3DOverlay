#include "D3DOverlay.hpp"

#define Log(x) printf(_("[D3LOG] %s\n"), x)

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace D3DOverlay
{
	std::function<void(int, int)> COverlay::m_UserRender;

	HWND COverlay::m_Window;
	HWND COverlay::m_TargetWindow;
	WNDCLASSEX COverlay::m_WindowClass;
	int COverlay::m_WindowWidth;
	int COverlay::m_WindowHeight;
	RECT COverlay::m_OldRect;

	LPDIRECT3D9 COverlay::m_D3D;
	LPDIRECT3DDEVICE9 COverlay::m_D3Device;
	D3DPRESENT_PARAMETERS COverlay::m_D3Parameters;

	bool COverlay::Init(const HWND TargetWindow, LPCSTR WindowClassName, LPCSTR WindowName)
	{
		if (!TargetWindow || !IsWindow(TargetWindow))
			return false;

		m_TargetWindow = TargetWindow;

		if (!CreateOverlay(WindowClassName, WindowName))
		{
			CleanupOverlay();
			Log(_("Failed to create overlay"));
			return false;
		}

		if (!CreateDeviceD3D())
		{
			CleanupOverlay();
			Log(_("Failed creating D3D device"));
			return false;
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(m_Window);
		ImGui_ImplDX9_Init(m_D3Device);

		Log(_("Created overlay successfully"));

		return true;
	}

	bool COverlay::CreateOverlay(LPCSTR WindowClassName, LPCSTR WindowName)
	{
		m_WindowWidth = GetSystemMetrics(SM_CXSCREEN);
		m_WindowHeight = GetSystemMetrics(SM_CYSCREEN);

		ZeroMemory(&m_WindowClass, sizeof(WNDCLASSEX));
		m_WindowClass = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0, 0, GetModuleHandle(nullptr), NULL, NULL, NULL, NULL, WindowClassName, NULL };
		RegisterClassEx(&m_WindowClass);

		m_Window = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, m_WindowClass.lpszClassName, WindowName, WS_POPUP, 0, 0, m_WindowWidth, m_WindowHeight, 0, 0, 0, 0);
		if (!m_Window)
			return false;

		SetLayeredWindowAttributes(m_Window, 0, 0, LWA_ALPHA);
		SetLayeredWindowAttributes(m_Window, 0, RGB(0, 0, 0), LWA_COLORKEY);

		ShowWindow(m_Window, SW_SHOW);
		MARGINS Margins{ -1 };
		DwmExtendFrameIntoClientArea(m_Window, &Margins);
		UpdateWindow(m_Window);

		return true;
	}

	bool COverlay::CreateDeviceD3D()
	{
		if ((m_D3D = Direct3DCreate9(D3D_SDK_VERSION)) == 0)
			return false;

		ZeroMemory(&m_D3Parameters, sizeof(m_D3Parameters));
		m_D3Parameters.Windowed = TRUE;
		m_D3Parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
		m_D3Parameters.BackBufferFormat = D3DFMT_UNKNOWN;
		m_D3Parameters.EnableAutoDepthStencil = TRUE;
		m_D3Parameters.AutoDepthStencilFormat = D3DFMT_D16;
		m_D3Parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

		if (m_D3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_Window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_D3Parameters, &m_D3Device) < 0)
			return false;

		return true;
	}

	bool COverlay::Update(bool MenuOpen)
	{
		if (m_Window)
		{
			MSG Msg{ 0 };

			if (::PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&Msg);
				::DispatchMessage(&Msg);
			}

			if (!m_TargetWindow || !IsWindow(m_TargetWindow))
				return false;

			if (MenuOpen)
			{
				SetWindowLong(m_Window, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW);
			}
			else
			{
				SetWindowLong(m_Window, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
			}

			ImGui_ImplDX9_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			if (m_UserRender)
				m_UserRender(m_WindowWidth, m_WindowHeight);

			DWORD color_write = 0UL, srgb_write = 0UL;
			IDirect3DVertexDeclaration9* vertex_declaration = nullptr;
			IDirect3DVertexShader9* vertex_shader = nullptr;
			m_D3Device->GetRenderState(D3DRS_COLORWRITEENABLE, &color_write);
			m_D3Device->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgb_write);
			m_D3Device->GetVertexDeclaration(&vertex_declaration);
			m_D3Device->GetVertexShader(&vertex_shader);

			m_D3Device->SetVertexShader(nullptr);
			m_D3Device->SetPixelShader(nullptr);
			m_D3Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
			m_D3Device->SetRenderState(D3DRS_LIGHTING, false);
			m_D3Device->SetRenderState(D3DRS_FOGENABLE, false);
			m_D3Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			m_D3Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

			m_D3Device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
			m_D3Device->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
			m_D3Device->SetRenderState(D3DRS_ZWRITEENABLE, false);
			m_D3Device->SetRenderState(D3DRS_STENCILENABLE, false);

			m_D3Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, false);
			m_D3Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, false);

			m_D3Device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			m_D3Device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
			m_D3Device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);
			m_D3Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_D3Device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA);
			m_D3Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			m_D3Device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);

			m_D3Device->SetRenderState(
				D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
			m_D3Device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

			ImGui::EndFrame();
			ImGui::Render();

			m_D3Device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.f, 0);

			//	If it's not D3D_OK
			if (m_D3Device->BeginScene() >= 0)
			{
				ImGui::Render();
				ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
				m_D3Device->EndScene();
			}

			m_D3Device->Present(NULL, NULL, NULL, NULL);

			m_D3Device->SetRenderState(D3DRS_COLORWRITEENABLE, color_write);
			m_D3Device->SetRenderState(D3DRS_SRGBWRITEENABLE, srgb_write);

			return Msg.message != WM_QUIT;
		}

		return false;
	}

	LRESULT __stdcall COverlay::WndProc(HWND aHwnd, UINT aMsg, WPARAM aWParam, LPARAM aLParam)
	{
		switch (aMsg)
		{
			case WM_DESTROY:
			{
				PostQuitMessage(0);
				break;
			}

			case WM_SYSCOMMAND:
			{
				if ((aWParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
					return S_OK;
				break;
			}
		}

		if (ImGui_ImplWin32_WndProcHandler(aHwnd, aMsg, aWParam, aLParam))
			return S_OK;

		return ::DefWindowProc(aHwnd, aMsg, aWParam, aLParam);
	}

	void COverlay::SetUserRender(const std::function<void(int, int)> UserRender)
	{
		m_UserRender = UserRender;
	}

	void COverlay::CleanupOverlay()
	{
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		if (m_D3Device)
			m_D3Device->Release();

		if (m_D3D)
			m_D3D->Release();

		if (m_Window)
			DestroyWindow(m_Window);

		UnregisterClass(m_WindowClass.lpszClassName, m_WindowClass.hInstance);
	}
}