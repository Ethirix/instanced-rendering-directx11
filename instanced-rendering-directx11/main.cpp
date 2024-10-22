#include <Windows.h>

#include "Engine/Engine.h"

Engine* CEngine = new Engine();

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
			continue;
		}

		CEngine->Update();
		CEngine->Draw();
	}

	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(const HWND hwnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
	PAINTSTRUCT ps{};
	HDC hdc{};

	switch (message)
	{
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_SIZE:
	case WM_EXITSIZEMOVE:
		//TODO: WM_SIZE Message
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}