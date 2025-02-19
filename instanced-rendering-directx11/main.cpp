#include <Windows.h>

#include "Engine/Globals.h"

#include "Optick/optick.h"

#define FAIL_CHECK if (FAILED(hr)) { delete CEngine; return hr; }

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	OPTICK_START_CAPTURE();
	HRESULT hr = CEngine->Initialise(hInstance); FAIL_CHECK

	MSG msg{};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else 
		{
			OPTICK_FRAME("MainThread");
			hr = CEngine->Update(); FAIL_CHECK
			hr = CEngine->Draw(); FAIL_CHECK
			
		}
	}

	OPTICK_STOP_CAPTURE();

#ifdef _INSTANCED_RENDERER
constexpr int INSTANCED = 1;
#else
constexpr int INSTANCED = 0;
#endif
#ifdef _INSTANCED_INPUT_LAYOUT
constexpr int INSTANCED_IA = 1;
#else
constexpr int INSTANCED_IA = 0;
#endif

	std::string fileName = std::format("i{}_ia{}_oc{}_ow{}_ou{}_lod1[{}]_lod2[{}].opt", INSTANCED, INSTANCED_IA, OBJECTS_TO_RENDER, OBJECTS_WIDTH_COUNT, OBJECTS_UNIT_SIZE, LOD_1, LOD_2);
	OPTICK_SAVE_CAPTURE(fileName.c_str());
	OPTICK_SHUTDOWN();
	delete CEngine;

	return static_cast<int>(msg.wParam);
}