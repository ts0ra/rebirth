#include "Windows.h"
#include "overlay.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    
    overlay esp(hInstance);
	esp.showOverlay();
	esp.runMessageLoop();

    return 0;
}
