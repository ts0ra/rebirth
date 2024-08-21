#pragma once
#include <d3d9.h>

class overlay
{
public:
    overlay(HINSTANCE hInst);  // init
    ~overlay();

    void runMessageLoop();

    void showOverlay();
    void hideOverlay();
private:
    HINSTANCE hInst;
    WNDCLASS wc;
    bool isRunning;

    HWND hWnd;
    LPDIRECT3D9 pD3D;
    LPDIRECT3DDEVICE9 pD3DDevice;
    D3DPRESENT_PARAMETERS d3dpp;

    void registerClassOverlay();
    void createOverlay();

    void createD3D();
    void initD3D();
    void cleanupD3D();

    void startRender();
    void rendering();
    void handleLostDevice();

    void drawMainMenu();
};