#pragma once
#include <windows.h>

class Engine
{
public:
	HRESULT Initialise(HINSTANCE hInstance);

	void Update();
	void Draw();
};

