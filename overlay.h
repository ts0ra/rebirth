#pragma once
#include <d3d9.h>
#include <log.h>
#include <memory.h>

class Overlay
{
public:
    bool isRunning{ true };
    Overlay(HINSTANCE hInst);  // init
    ~Overlay();

    void runMessageLoop();

    void showOverlay();
    void hideOverlay();

private:
    HINSTANCE hInst;
    WNDCLASS wc{};
    MEMORY mem;

    HWND hWnd{};
    LPDIRECT3D9 pD3D{};
    LPDIRECT3DDEVICE9 pD3DDevice{};
    D3DPRESENT_PARAMETERS d3dpp{};

    RECT clientRect{ 0, 0, 0, 0 };
    POINT clientToScreenPoint{ 0, 0 };

    int clientWidth{ 500 };
    int clientHeight{ 300 };

    void registerClassOverlay();
    void createOverlay();

    void createD3D();
    void initD3D();
    void cleanupD3D();

    void startRender();
    void rendering();
    void handleLostDevice();

    void drawESP();
    void drawMainMenu();

    void hack();
};