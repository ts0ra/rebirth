#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "overlay.h"

#include <stdexcept>
#include <sstream>

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

overlay::overlay(HINSTANCE hInst)
{
	hInst = { hInst };
	wc = {};
	isRunning = { true };

	pD3D = {};
	pD3DDevice = {};
	hWnd = {};
	d3dpp = {};

	registerClassOverlay();
	createOverlay();
	createD3D();
	hideOverlay();
	initD3D();
}

overlay::~overlay()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	cleanupD3D();
	DestroyWindow(hWnd);
}

void overlay::registerClassOverlay()
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

void overlay::createOverlay()
{
	/*HWND targetWindow = FindWindowA(NULL, "AssaultCube");

	RECT clientRect{ 0, 0, 800, 600 };
	POINT clientToScreenPoint = { 0, 0 };

	if (GetClientRect(targetWindow, &clientRect)) {
		ClientToScreen(targetWindow, &clientToScreenPoint);
	}

	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;*/

	hWnd = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
		wc.lpszClassName,
		L"ESP Overlay",
		WS_POPUP,
		0,
		0,
		500,
		300,
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

void overlay::showOverlay()
{
	ShowWindow(hWnd, SW_SHOW);
}

void overlay::hideOverlay()
{
	ShowWindow(hWnd, SW_HIDE);
}

void overlay::runMessageLoop()
{
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

		// main loop
		handleLostDevice();
		startRender();
		// esp view port
		drawESP();
		// main menu view port
		drawMainMenu();
		rendering();
	}
}

void overlay::createD3D()
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

void overlay::initD3D()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // use viewport

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

void overlay::cleanupD3D()
{
	if (pD3DDevice) { pD3DDevice->Release(); pD3DDevice = nullptr; }
	if (pD3D) { pD3D->Release(); pD3D = nullptr; }
}

void overlay::startRender()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void overlay::rendering()
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

void overlay::handleLostDevice()
{
	HRESULT result = pD3DDevice->Present(nullptr, nullptr, nullptr, nullptr);
	if (result == D3DERR_DEVICELOST && pD3DDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		HRESULT hr = pD3DDevice->Reset(&d3dpp); // Use the member variable d3dpp here
		if (hr == D3DERR_INVALIDCALL)
			IM_ASSERT(0);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

void overlay::drawESP()
{
	ImDrawList* mainDraw = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
	ImVec2 mainViewPortPos = ImGui::GetMainViewport()->Pos;
	mainDraw->AddText(ImGui::GetMainViewport()->Pos, IM_COL32(255, 255, 255, 255), "Rebirth");
	mainDraw->AddRectFilled(ImVec2(mainViewPortPos.x + 100, mainViewPortPos.y + 100), ImVec2(mainViewPortPos.x + 200, mainViewPortPos.y + 200), ImColor(255, 255, 255), 0);
}

void overlay::drawMainMenu()
{
	ImGui::Begin("Main Menu");
	ImGui::Text("Hello, world!");
	ImGui::End();
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
		return true;
	}

	switch (uMsg)
	{
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