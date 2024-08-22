#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <d3dx9.h>

#include <overlay.h>
#include <ImVec2Operators.h>
#include <hack.h>
#include <offset.h>

#include <stdexcept>
#include <sstream>
#include <thread>
#include <cmath>
//#include <chrono>

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void checkProcess(Memory& memory, HWND& hWnd, RECT& clientRect, POINT& clientToScreenPoint, int& clientWidth, int& clientHeight);
bool WorldToScreen(const D3DXVECTOR3& pos, D3DXVECTOR3& screen, const D3DMATRIX& matrix, int width, int height);
float GetAngleDifference(float angle1, float angle2);
bool IsTargetWithinFOV(const D3DXVECTOR3& ourViewAngles, const D3DXVECTOR3& ourTargetViewAngles, float maxFOV);
D3DXVECTOR3 SmoothAim(const D3DXVECTOR3& currentViewAngles, const D3DXVECTOR3& targetViewAngles, float smoothingFactor);
D3DXVECTOR3 CalculateAngle(const D3DXVECTOR3& source, const D3DXVECTOR3& destination);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Global variables
static bool g_ForceExitThread = { false };
static HWND g_TargetWindow = { nullptr };
static UINT g_ResizeWidth{ 0 }, g_ResizeHeight{ 0 };

// Thread
std::thread checkProcessThread;
std::thread getHacksValueThread;

Overlay::Overlay(HINSTANCE hInst) : mem(L"ac_client.exe")
{
	hInst = { hInst };

	if (mem.GetProcessId() != 0)
	{
		g_TargetWindow = FindWindow(NULL, L"AssaultCube");
		data::baseAddress = { mem.GetModuleAddress(L"ac_client.exe") };
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

	getHacksValueThread = std::thread(&Overlay::getHacksValue, this);

	while (isRunning)
	{
		//auto start = std::chrono::high_resolution_clock::now(); // Start time

		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				isRunning = !isRunning;
				return;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		HandleOverlayVisibility();
		checkTargetWindowPosition();

		// main loop
		handleLostDevice(deviceLost, deviceLostTrigger);
		handleResize();

		startRender();

		hack();
		drawESP();
		drawMainMenu();

		rendering();

		checkIsDeviceLost(deviceLost);

		//auto end = std::chrono::high_resolution_clock::now(); // End time
		//std::chrono::duration<double, std::milli> duration = end - start; // Calculate duration in milliseconds
		//appLog.AddLog("Iteration took %.2f ms\n", duration.count()); // Log the duration
	}

	g_ForceExitThread = { true };
	if (getHacksValueThread.joinable()) {
		getHacksValueThread.join();
	}
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
	if (mem.GetProcessId() == 0)
		return;

	g_TargetWindow = FindWindow(NULL, L"AssaultCube");
	RECT currentClientRect{};
	POINT currentClientToScreenPoint{};
	GetClientRect(g_TargetWindow, &currentClientRect);
	ClientToScreen(g_TargetWindow, &currentClientToScreenPoint);

	if (currentClientToScreenPoint.x != clientToScreenPoint.x || currentClientToScreenPoint.y != clientToScreenPoint.y)
	{
		clientWidth = { currentClientRect.right - currentClientRect.left };
		clientHeight = { currentClientRect.bottom - currentClientRect.top };

		clientToScreenPoint = { currentClientToScreenPoint };
		SetWindowPos(
			hWnd,
			HWND_TOPMOST,
			clientToScreenPoint.x,
			clientToScreenPoint.y,
			clientWidth,
			clientHeight,
			SWP_NOACTIVATE
		);
	}
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
	// Window main viewport
	ImDrawList* mainDraw = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
	ImVec2 mainViewPortPos = ImGui::GetMainViewport()->Pos;

	// Display overlay FPS
	char fpsText[16];  // Sufficient size for FPS display
	sprintf_s(fpsText, sizeof(fpsText), "FPS: %d", static_cast<int>(ImGui::GetIO().Framerate));
	mainDraw->AddText(mainViewPortPos + ImVec2(5, 5), ImColor(255, 255, 255), fpsText);

	if (hack::toggleDrawFOV)
	{
		ImVec2 screenCenter(clientWidth / 2.0f, clientHeight / 2.0f);
		float scalingFactor = 7.0f / 1.0f; 
		float fov = hack::aimbotFOV * scalingFactor;

		// Draw fov
		if (hack::fovType == 0)
			mainDraw->AddCircle(mainViewPortPos + screenCenter, fov, ImColor(hack::fovColor[0], hack::fovColor[1], hack::fovColor[2]));
		else
		{
			float diameter = 2 * fov;
			ImVec2 topLeft = mainViewPortPos + screenCenter - ImVec2(fov, fov);
			ImVec2 bottomRight = mainViewPortPos + screenCenter + ImVec2(fov, fov);
			mainDraw->AddRect(topLeft, bottomRight, ImColor(hack::fovColor[0], hack::fovColor[1], hack::fovColor[2]));
		}
	}


	// Player ESP
	if (hack::toggleESP)
	{
		for (int i{ 0 }; i < data::totalPlayers; ++i)
		{
			uintptr_t entity = mem.Read<uintptr_t>(data::playersEntityList + (i * 0x4));
			if (entity == 0) continue; // Skip if invalid entity

			if (hack::isTeamGameMode(data::gameMode))
			{
				int teamSide = mem.Read<int>(entity + offsets::teamSide);
				int localTeamSide = mem.Read<int>(data::localPlayerEntity + offsets::teamSide);
				if (teamSide == localTeamSide) continue; // Skip if the entity is on the same team
			}

			bool isDead = mem.Read<bool>(entity + offsets::isDead);
			if (isDead) continue; // Skip if the entity is dead

			D3DXVECTOR3 headPosition = mem.Read<D3DXVECTOR3>(entity + offsets::head);
			D3DXVECTOR3 footPosition = mem.Read<D3DXVECTOR3>(entity + offsets::foot);
			D3DXVECTOR3 headScreenPosition, footScreenPosition, screenPosition;

			if (WorldToScreen(headPosition, headScreenPosition, data::viewMatrix, clientWidth, clientHeight) &&
				WorldToScreen(footPosition, footScreenPosition, data::viewMatrix, clientWidth, clientHeight))
			{

				// Draw player box ESP
				float boxHeight = footScreenPosition.y - headScreenPosition.y;
				float boxWidth = boxHeight / 2.0f; // Assuming a 1:2 width to height ratio
				if (hack::toggleBoxESP)
				{
					mainDraw->AddRect(
						mainViewPortPos + ImVec2(headScreenPosition.x - boxWidth / 2, headScreenPosition.y), // Top-left corner
						mainViewPortPos + ImVec2(headScreenPosition.x + boxWidth / 2, footScreenPosition.y), // Bottom-right corner
						ImColor(255, 0, 0)
					);
				}

				// Draw player name ESP
				if (hack::toggleNameESP)
				{
					char playerName[16];
					mem.ReadChar<char>(entity + offsets::name, playerName, 16);

					ImVec2 textSize = ImGui::CalcTextSize(playerName);
					ImVec2 textPosition = mainViewPortPos + ImVec2(headScreenPosition.x - textSize.x / 2, headScreenPosition.y - 15.0f);
					mainDraw->AddText(textPosition, ImColor(255, 255, 255), playerName);
				}

				// Draw health bar ESP
				float health = static_cast<float>(mem.Read<int>(entity + offsets::health));
				float maxHealth = 100.0f; // Max health
				float healthBarHeight = boxHeight * (health / maxHealth); // Health bar height proportional to the player height
				float healthBarWidth = 2.0f; // Width of the health bar

				ImVec2 healthBarTopLeft = ImVec2((headScreenPosition.x - boxWidth / 2 - healthBarWidth) - 2, headScreenPosition.y);
				ImVec2 healthBarBottomRight = ImVec2(healthBarTopLeft.x + healthBarWidth, headScreenPosition.y + boxHeight);

				ImVec2 healthBarForegroundTopLeft = mainViewPortPos + ImVec2(healthBarTopLeft.x, healthBarTopLeft.y + (boxHeight - healthBarHeight));
				ImVec2 healthBarForegroundBottomRight = mainViewPortPos + ImVec2(healthBarTopLeft.x + healthBarWidth, healthBarTopLeft.y + boxHeight);
				if (hack::toggleHealthESP)
				{
					mainDraw->AddRectFilled(healthBarTopLeft, healthBarBottomRight, ImColor(0, 0, 0));
					mainDraw->AddRectFilled(healthBarForegroundTopLeft, healthBarForegroundBottomRight, ImColor(0, 255, 0));
				}

				// Draw armor bar ESP
				float armor = static_cast<float>(mem.Read<int>(entity + offsets::armor));
				float maxArmor = 100.0f; // Adjust based on game mechanics
				float armorBarHeight = boxHeight * (armor / maxArmor); // Armor bar height proportional to the player height
				float armorBarWidth = 2.0f; // Width of the armor bar

				ImVec2 armorBarTopLeft = ImVec2(healthBarTopLeft.x - armorBarWidth - 2, headScreenPosition.y);
				ImVec2 armorBarBottomRight = ImVec2(armorBarTopLeft.x + armorBarWidth, headScreenPosition.y + boxHeight);

				ImVec2 armorBarForegroundTopLeft = mainViewPortPos + ImVec2(armorBarTopLeft.x, armorBarTopLeft.y + (boxHeight - armorBarHeight));
				ImVec2 armorBarForegroundBottomRight = mainViewPortPos + ImVec2(armorBarTopLeft.x + armorBarWidth, armorBarTopLeft.y + boxHeight);
				if (hack::toggleArmorESP)
				{
					mainDraw->AddRectFilled(armorBarTopLeft, armorBarBottomRight, ImColor(0, 0, 0));
					mainDraw->AddRectFilled(armorBarForegroundTopLeft, armorBarForegroundBottomRight, ImColor(255, 255, 255));
				}

				// Draw player distance ESP
				if (hack::toggleDistaneESP)
				{
					D3DXVECTOR3 localHeadPosition = mem.Read<D3DXVECTOR3>(data::localPlayerEntity + offsets::head);
					D3DXVECTOR3 distanceVector = headPosition - localHeadPosition;
					float distance = D3DXVec3Length(&distanceVector);

					char distanceText[16];
					sprintf_s(distanceText, sizeof(distanceText), "%d m", static_cast<int>(distance));

					ImVec2 distanceTextSize = ImGui::CalcTextSize(distanceText);
					ImVec2 distanceTextPosition = mainViewPortPos + ImVec2(headScreenPosition.x - distanceTextSize.x / 2, footScreenPosition.y + 1.0f);
					mainDraw->AddText(distanceTextPosition, ImColor(255, 255, 255), distanceText);
				}

			}
		}
	}
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
			ImGui::Checkbox("Aimbot", &hack::toggleAimbot); ImGui::SameLine();
			ImGui::Checkbox("Enable Smoothing", &hack::toggleSmoothAim);
			ImGui::SliderFloat("Smoothing", &hack::aimbotSmoothing, 1.0f, 10.0f);
			ImGui::SliderFloat("Aimbot FOV", &hack::aimbotFOV, 1.0f, 180.0f);
			ImGui::Checkbox("No Recoil", &hack::toggleNoRecoil);
			ImGui::Checkbox("Unlimited Ammo", &hack::toggleUnlimitedAmmo);
			ImGui::Checkbox("Unlimited Health", &hack::toggleUnlimitedHealth);
			ImGui::Checkbox("Unlimited Armor", &hack::toggleUnlimitedArmor);
			ImGui::Checkbox("Radar", &hack::toggleRadar);

			ImGui::EndTabItem();
		}

		// Visual tab
		if (ImGui::BeginTabItem("visual"))
		{
			ImGui::Checkbox("Draw FOV", &hack::toggleDrawFOV); ImGui::SameLine();
			ImGui::RadioButton("Circle", &hack::fovType, 0); ImGui::SameLine();
			ImGui::RadioButton("Rectangle", &hack::fovType, 1); ImGui::SameLine();
			ImGui::ColorEdit3("FOV Color", hack::fovColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);

			if (!g_TargetWindow) ImGui::BeginDisabled();
			ImGui::Checkbox("ESP", &hack::toggleESP);
			if (!g_TargetWindow) ImGui::EndDisabled();

			if (!hack::toggleESP) ImGui::BeginDisabled();
			ImGui::Checkbox("Box", &hack::toggleBoxESP);
			ImGui::Checkbox("Player Name", &hack::toggleNameESP);
			ImGui::Checkbox("Health", &hack::toggleHealthESP);
			ImGui::Checkbox("Armor", &hack::toggleArmorESP);
			ImGui::Checkbox("Player Distance", &hack::toggleDistaneESP);
			if (!hack::toggleESP) ImGui::EndDisabled();

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
			// show mainviewport pos and size
			ImGui::Text("MainViewport Pos: (%.0f, %.0f)", ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y);
			ImGui::Text("MainViewport Size: (%.0f, %.0f)", ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Size.y);

			// Show frametime
			//ImGui::Text("Frametime: %.3f ms | FPS: %.0f", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			// show plot frametime
			//static float values[90] = { 0 };
			//static int values_offset = 0;
			//values[values_offset] = 1000.0f / ImGui::GetIO().Framerate;
			//values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
			//ImGui::PlotLines("Frametime", values, IM_ARRAYSIZE(values), values_offset, NULL, 0.0f, 20.0f, ImVec2(0, 80));

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

void Overlay::hack()
{
	if (hack::toggleAimbot)
	{
		if (!data::localIsDead)
		{

			float smallestAngleDifference = FLT_MAX; // Initialize with the maximum possible float value
			D3DXVECTOR3 closestTargetViewAngle;

			for (int i{ 0 }; i < data::totalPlayers; ++i)
			{
				// Skip if invalid tempPlayerEntity
				uintptr_t tempPlayerEntity = { mem.Read<uintptr_t>(data::playersEntityList + (i * 0x4)) };
				if (tempPlayerEntity == 0) continue;

				// Skip if the tempPlayerEntity is dead
				bool tempEntityIsDead = { mem.Read<bool>(tempPlayerEntity + offsets::isDead) };
				if (tempEntityIsDead) continue;

				// Skip if tempPlayerEntity is on the same team
				int tempEntityTeamSide = { mem.Read<int>(tempPlayerEntity + offsets::teamSide) };
				if (tempEntityTeamSide == data::localTeamSide) continue;

				D3DXVECTOR3 tempEntityHeadPos = { mem.Read<D3DXVECTOR3>(tempPlayerEntity + offsets::head) };
				D3DXVECTOR3 targetViewAngle = { CalculateAngle(data::localPlayerHeadPos, tempEntityHeadPos) };

				if (IsTargetWithinFOV(data::viewAngle, targetViewAngle, hack::aimbotFOV))
				{
					float angleDifference = GetAngleDifference(data::viewAngle.y, targetViewAngle.y) + 
											GetAngleDifference(data::viewAngle.x, targetViewAngle.x);

					if (angleDifference < smallestAngleDifference)
					{
						smallestAngleDifference = angleDifference;
						closestTargetViewAngle = targetViewAngle;
					}

				}
			}
			if (smallestAngleDifference < FLT_MAX && (GetAsyncKeyState(VK_RBUTTON) & 0x8000))
			{
				D3DXVECTOR3 smoothedAngles = SmoothAim(data::viewAngle, closestTargetViewAngle, hack::aimbotSmoothing);
				if (hack::toggleSmoothAim)
					mem.Write<D3DXVECTOR3>(data::localPlayerEntity + offsets::viewAngles, smoothedAngles);
				else
					mem.Write<D3DXVECTOR3>(data::localPlayerEntity + offsets::viewAngles, closestTargetViewAngle);
			}
		}
	}

	if (hack::toggleUnlimitedAmmo)
	{
		mem.Write<int>(mem.Read<std::uintptr_t>(data::localPlayerEntity, { offsets::currentWeaponObject, offsets::ammoLoaded }), 100);
	}

	if (hack::toggleUnlimitedHealth)
	{
		mem.Write<int>(data::localPlayerEntity + offsets::health, 999);
	}

	if (hack::toggleUnlimitedArmor)
	{
		mem.Write<int>(data::localPlayerEntity + offsets::armor, 999);
	}

	if (hack::toggleNoRecoil)
	{
		mem.PatchEx((BYTE*)(data::baseAddress + offsets::noRecoil), (BYTE*)("\xC2\x08\x00"), 3);
	}
	else
	{
		mem.PatchEx((BYTE*)(data::baseAddress + offsets::noRecoil), (BYTE*)("\x83\xEC\x28"), 3);
	}

	if (hack::toggleRadar)
	{
		mem.PatchEx((BYTE*)(data::baseAddress + offsets::radarMap), (BYTE*)("\xE9\x1D\x01\x00\x00\x90"), 6);
		mem.PatchEx((BYTE*)(data::baseAddress + offsets::radarMiniMap), (BYTE*)("\xE9\x2B\x01\x00\x00\x90"), 6);
	}
	else
	{
		mem.PatchEx((BYTE*)(data::baseAddress + offsets::radarMap),		(BYTE*)("\x0F\x8D\xCC\x00\x00\x00"), 6);
		mem.PatchEx((BYTE*)(data::baseAddress + offsets::radarMiniMap), (BYTE*)("\x0F\x8D\xD5\x00\x00\x00"), 6);
	}
}

void Overlay::getHacksValue()
{
	while (!g_ForceExitThread)
	{
		if (!g_TargetWindow)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		data::localPlayerEntity = { mem.Read<uintptr_t>(data::baseAddress + offsets::localPlayerEntity) };
		data::playersEntityList = { mem.Read<uintptr_t>(data::baseAddress + offsets::playersEntityList) };
		data::totalPlayers = { mem.Read<int>(data::baseAddress + offsets::totalPlayer) };
		data::viewMatrix = { mem.Read<D3DMATRIX>(data::baseAddress + offsets::viewMatrix) };
		data::gameMode = { mem.Read<int>(data::baseAddress + offsets::gameMode) };
		data::localPlayerHeadPos = { mem.Read<D3DXVECTOR3>(data::localPlayerEntity + offsets::head) };
		data::localTeamSide = { mem.Read<int>(data::localPlayerEntity + offsets::teamSide) };
		data::localIsDead = { mem.Read<bool>(data::localPlayerEntity + offsets::isDead) };
		data::viewAngle = { mem.Read<D3DXVECTOR3>(data::localPlayerEntity + offsets::viewAngles) };
	}
}

void Overlay::HandleOverlayVisibility()
{
	HWND foregroundWindow = GetForegroundWindow();

	if (foregroundWindow == g_TargetWindow)
	{
		showOverlay();
	}
	else
	{
		hideOverlay();
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

void checkProcess(Memory& mem, HWND& hWnd, RECT& clientRect, POINT& clientToScreenPoint, int& clientWidth, int& clientHeight) {
	while (!g_ForceExitThread)
	{
		appLog.AddLog("[-] Process not found, retrying in 5 seconds...\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		mem.Reinitialize(L"ac_client.exe");

		if (mem.GetProcessId() != 0)
		{
			data::baseAddress = { mem.GetModuleAddress(L"ac_client.exe") };

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

bool WorldToScreen(const D3DXVECTOR3& pos, D3DXVECTOR3& screen, const D3DMATRIX& matrix, int width, int height) {
	D3DXVECTOR4 clipCoords;
	clipCoords.x = pos.x * matrix._11 + pos.y * matrix._21 + pos.z * matrix._31 + matrix._41;
	clipCoords.y = pos.x * matrix._12 + pos.y * matrix._22 + pos.z * matrix._32 + matrix._42;
	clipCoords.z = pos.x * matrix._13 + pos.y * matrix._23 + pos.z * matrix._33 + matrix._43;
	clipCoords.w = pos.x * matrix._14 + pos.y * matrix._24 + pos.z * matrix._34 + matrix._44;

	if (clipCoords.w < 0.1f) return false;

	D3DXVECTOR3 NDC;
	NDC.x = clipCoords.x / clipCoords.w;
	NDC.y = clipCoords.y / clipCoords.w;
	NDC.z = clipCoords.z / clipCoords.w;

	screen.x = (width / 2 * NDC.x) + (NDC.x + width / 2);
	screen.y = -(height / 2 * NDC.y) + (NDC.y + height / 2);
	return true;
}

float GetAngleDifference(float angle1, float angle2)
{
	float diff = angle1 - angle2;
	while (diff > 180.0f) diff -= 360.0f;
	while (diff < -180.0f) diff += 360.0f;
	return fabs(diff);
}

bool IsTargetWithinFOV(const D3DXVECTOR3& ourViewAngles, const D3DXVECTOR3& ourTargetViewAngles, float maxFOV)
{
	float yawDifference = GetAngleDifference(ourViewAngles.y, ourTargetViewAngles.y);
	float pitchDifference = GetAngleDifference(ourViewAngles.x, ourTargetViewAngles.x);

	return (yawDifference <= maxFOV && pitchDifference <= maxFOV);
}

D3DXVECTOR3 SmoothAim(const D3DXVECTOR3& currentViewAngles, const D3DXVECTOR3& targetViewAngles, float smoothingFactor)
{
	D3DXVECTOR3 smoothedAngles;
	smoothedAngles.x = currentViewAngles.x + (targetViewAngles.x - currentViewAngles.x) / smoothingFactor;
	smoothedAngles.y = currentViewAngles.y + (targetViewAngles.y - currentViewAngles.y) / smoothingFactor;
	smoothedAngles.z = 0.0f;
	return smoothedAngles;
}

D3DXVECTOR3 CalculateAngle(const D3DXVECTOR3& source, const D3DXVECTOR3& destination)
{
	D3DXVECTOR3 delta = destination - source;
	float hypotenuse = sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
	D3DXVECTOR3 angles;
	angles.x = atan2f(delta.y, delta.x) * (180.0f / D3DX_PI);
	angles.y = atan2f(delta.z, hypotenuse) * (180.0f / D3DX_PI);
	angles.z = 0.0f;

	angles.x += 90; // need to add this otherwise viewangle will be offside 90 degree

	if (angles.x < 0.0f)
	{
		angles.x += 360.0f; // normalized because if it's negative, it will be off by 360 degree (this fix flicker issue)
	}

	return angles;
}