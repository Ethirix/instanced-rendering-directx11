#pragma once
#include <DirectXMath.h>

struct CBCamera
{
	DirectX::XMMATRIX Projection;
	DirectX::XMMATRIX View;
	DirectX::XMFLOAT4 CameraPosition;
	
	DirectX::XMFLOAT4 __Buffer1, __Buffer2, __Buffer3;  
};
