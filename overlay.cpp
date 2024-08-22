#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include <overlay.h>
#include <ImVec2Operators.h>

#include <stdexcept>
#include <sstream>
#include <thread>

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void checkProcess(Memory& memory, HWND& hWnd, RECT& clientRect, POINT& clientToScreenPoint, int& clientWidth, int& clientHeight);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Global variables
static bool g_ForceExitThread = { false };
static HWND g_TargetWindow = { nullptr };
static UINT g_ResizeWidth{ 0 }, g_ResizeHeight{ 0 };

// Thread
std::thread checkProcessThread;

Overlay::Overlay(HINSTANCE hInst) : mem(L"ac_client.exe")
{
	hInst = { hInst };

	if (mem.GetProcessId() != 0)
	{
		g_TargetWindow = FindWindow(NULL, L"AssaultCube");
	}
	else
	{
		checkProcessThread = std::thread(
			checkProcess, std::ref(mem),
			std::ref(hWnd),
			std::ref(clientRect),
			std::ref(clientToScreenPoint),
			std::ref(clientWidth),
			std::ref(clientHeight)
		);
	}

	/*menuClass.ViewportFlagsOverrideSet = {
		ImGuiViewportFlags_NoAutoMerge |
		ImGuiViewportFlags_IsFocused
	};*/

	registerClassOverlay();
	createOverlay();
	createD3D();
	hideOverlay();
	initD3D();
}

Overlay::~Overlay()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	cleanupD3D();
	DestroyWindow(hWnd);
}

void Overlay::registerClassOverlay()
{
	wc.style = { 0 };
	wc.lpfnWndProc = { WindowProc };
	wc.cbClsExtra = { 0L };
	wc.cbWndExtra = { 0L };
	wc.hInstance = { hInst };
	wc.hIcon = { NULL };
	wc.hCursor = { NULL };
	wc.hbrBackground = { NULL };
	wc.lpszMenuName = { NULL };
	wc.lpszClassName = { L"ESP" };

	RegisterClass(&wc);
}

void Overlay::createOverlay()
{
	if (g_TargetWindow)
	{
		if (GetClientRect(g_TargetWindow, &clientRect)) {
			ClientToScreen(g_TargetWindow, &clientToScreenPoint);
		}

		clientWidth = { clientRect.right - clientRect.left };
		clientHeight = { clientRect.bottom - clientRect.top };
	}

	hWnd = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
		wc.lpszClassName,
		L"ESP Overlay",
		WS_POPUP,
		clientToScreenPoint.x,
		clientToScreenPoint.y,
		clientWidth,
		clientHeight,
		NULL,
		NULL,
		hInst,
		NULL
	);

	if (hWnd == NULL)
	{
		/*DWORD error = GetLastError();
		std::wstringstream ss;
		ss << L"Failed to create main window. Error code: " << error;
		std::wstring errorMessage = ss.str();
		MessageBoxW(NULL, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);*/
		assert(false && "Failed to create main window");
		throw std::runtime_error("Failed to create main window");
	}

	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
}

void Overlay::showOverlay()
{
	ShowWindow(hWnd, SW_SHOW);
}

void Overlay::hideOverlay()
{
	ShowWindow(hWnd, SW_HIDE);
}

void Overlay::runMessageLoop()
{
	bool deviceLostTrigger { false };
	bool deviceLost { false };
	MSG msg{};
	while (isRunning)
	{
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				isRunning = !isRunning;
				return;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		checkTargetWindowPosition();

		// main loop
		handleLostDevice(deviceLost, deviceLostTrigger);
		handleResize();

		startRender();

		drawESP();
		drawMainMenu();

		rendering();

		checkIsDeviceLost(deviceLost);
	}

	g_ForceExitThread = { true };
	if (checkProcessThread.joinable()) {
		checkProcessThread.join();
	}
}

void Overlay::createD3D()
{
	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
	{
		cleanupD3D();
		assert(false && "Failed to create D3D object");
		throw std::runtime_error("Failed to create D3D device");
	}

	// Create the D3DDevice
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pD3DDevice) < 0)
	{
		cleanupD3D();
		assert(false && "Failed to create D3D device");
		throw std::runtime_error("Failed to create D3D device");
	}
}

void Overlay::initD3D()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // use viewport
	io.ConfigViewportsNoAutoMerge = true; // dont merge viewports automatically

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX9_Init(pD3DDevice);
}

void Overlay::cleanupD3D()
{
	if (pD3DDevice) { pD3DDevice->Release(); pD3DDevice = nullptr; }
	if (pD3D) { pD3D->Release(); pD3D = nullptr; }
}

void Overlay::startRender()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Overlay::rendering()
{
	ImGui::EndFrame();
	pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pD3DDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
	if (pD3DDevice->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		pD3DDevice->EndScene();
	}

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void Overlay::checkTargetWindowPosition()
{
	if (!g_TargetWindow)
		return;

	RECT rect;

}

void Overlay::handleLostDevice(bool& deviceLost, bool& deviceLostTrigger)
{
	HRESULT hr = pD3DDevice->TestCooperativeLevel();
	if (hr == D3DERR_DEVICELOST)
	{
		::Sleep(10);
		deviceLostTrigger = { true };
	}
	if (hr == D3DERR_DEVICENOTRESET)
		resetDevice();
	deviceLost = { false };
}

void Overlay::handleResize()
{
	if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
	{
		d3dpp.BackBufferWidth = g_ResizeWidth;
		d3dpp.BackBufferHeight = g_ResizeHeight;
		g_ResizeWidth = g_ResizeHeight = 0;
		resetDevice();
	}
}

void Overlay::resetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = pD3DDevice->Reset(&d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

void Overlay::checkIsDeviceLost(bool& deviceLost)
{
	HRESULT result = pD3DDevice->Present(nullptr, nullptr, nullptr, nullptr);
	if (result == D3DERR_DEVICELOST)
		deviceLost = { true };
}

void Overlay::drawESP()
{
	// Window viewport id
	ImDrawList* mainDraw = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
	ImVec2 mainViewPortPos = ImGui::GetMainViewport()->Pos;
	mainDraw->AddText(ImGui::GetMainViewport()->Pos, IM_COL32(255, 255, 255, 255), "Rebirth");
	mainDraw->AddRectFilled(mainViewPortPos + ImVec2(100.0f, 100.0f), mainViewPortPos + ImVec2(200.0f, 200.0f), ImColor(255, 255, 255), 0);
}

void Overlay::drawMainMenu()
{
	//ImGui::SetNextWindowClass(&menuClass);
	ImGui::SetNextWindowSize(ImVec2(500, 300));
	ImGui::Begin(
		"Rebirth",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDocking
	);

	if (ImGui::BeginTabBar("MainTab"))
	{
		// Hack tab
		if (ImGui::BeginTabItem("hack"))
		{
			ImGui::EndTabItem();
		}

		// Visual tab
		if (ImGui::BeginTabItem("visual"))
		{
			//if (memory.GetProcessId() == 0) ImGui::BeginDisabled();

			//ImGui::Checkbox("ESP");

			//if (memory.GetProcessId() == 0) ImGui::EndDisabled();

			ImGui::EndTabItem();
		}

		// Log tab
		if (ImGui::BeginTabItem("log"))
		{
			appLog.Draw("Rebirth");
			ImGui::EndTabItem();
		}

		// Test tab
		if (ImGui::BeginTabItem("test"))
		{
			ImGui::Text("Window Viewport ID: %d", ImGui::GetWindowViewport()->ID);

			ImVec2 mainViewPortPos = ImGui::GetMainViewport()->Pos;
			ImGui::Text("Main Viewport Pos: (%.1f, %.1f)", mainViewPortPos.x, mainViewPortPos.y);

			// ClientToScreenPoint
			ImGui::Text("ClientToScreenPoint: (%d, %d)", clientToScreenPoint.x, clientToScreenPoint.y);

			ImVec2 mainViewPortSize = ImGui::GetMainViewport()->Size;
			ImGui::Text("Main Viewport Size: (%.1f, %.1f)", mainViewPortSize.x, mainViewPortSize.y);

			ImVec2 windowPos = ImGui::GetWindowPos();
			ImGui::Text("Window Pos: (%.1f, %.1f)", windowPos.x, windowPos.y);

			ImVec2 windowSize = ImGui::GetWindowSize();
			ImGui::Text("Window Size: (%.1f, %.1f)", windowSize.x, windowSize.y);

			ImVec2 windowViewportPos = ImGui::GetWindowViewport()->Pos;
			ImGui::Text("Window Viewport Pos: (%.1f, %.1f)", windowViewportPos.x, windowViewportPos.y);

			ImVec2 windowViewportSize = ImGui::GetWindowViewport()->Size;
			ImGui::Text("Window Viewport Size: (%.1f, %.1f)", windowViewportSize.x, windowViewportSize.y);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

void Overlay::hack()
{

}

void checkProcess(Memory& memory, HWND& hWnd, RECT& clientRect, POINT& clientToScreenPoint, int& clientWidth, int& clientHeight) {
	while (!g_ForceExitThread)
	{
		appLog.AddLog("[-] Process not found, retrying in 5 seconds...\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		memory.Reinitialize(L"ac_client.exe");

		if (memory.GetProcessId() != 0)
		{
			g_TargetWindow = FindWindow(NULL, L"AssaultCube");

			if (GetClientRect(g_TargetWindow, &clientRect)) {
				ClientToScreen(g_TargetWindow, &clientToScreenPoint);
			}

			clientWidth = { clientRect.right - clientRect.left };
			clientHeight = { clientRect.bottom - clientRect.top };

			SetWindowPos(
				hWnd, 
				HWND_TOPMOST, 
				clientToScreenPoint.x,
				clientToScreenPoint.y,
				clientWidth,
				clientHeight,
				SWP_NOACTIVATE
			);
			break;
		}
	}
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
		return true;
	}

	switch (uMsg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)	// Disable ALT application menu
			return 0;							// (when you press ALT when window active, the cursor will move to the taskbar (top-left corner))
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}