#include <Windows.h>

#include "Engine/Globals.h"

#define FAIL_CHECK if (FAILED(hr)) { delete CEngine; return hr; }

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
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
			hr = CEngine->Update(); FAIL_CHECK
			hr = CEngine->Draw(); FAIL_CHECK
		}
	}

	delete CEngine;

	return static_cast<int>(msg.wParam);
}