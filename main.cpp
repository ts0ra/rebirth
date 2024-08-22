#include <overlay.h>
#include <Windows.h>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    Overlay esp{ hInstance };
    esp.showOverlay();
    esp.runMessageLoop();

    return 0;
}
