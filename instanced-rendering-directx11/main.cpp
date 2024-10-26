#include <Windows.h>

#include "Engine/Globals.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	if (HRESULT result = FAILED(CEngine->Initialise(hInstance)))
		return result;	

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
			CEngine->Update();
			CEngine->Draw();
		}
	}

	delete CEngine;

	return static_cast<int>(msg.wParam);
}