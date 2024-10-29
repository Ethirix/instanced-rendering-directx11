#pragma once
#include <DirectXMath.h>

class Camera
{
public:
	DirectX::XMFLOAT4X4 View, Projection;
	DirectX::XMFLOAT3 Eye, At, Up;

	float NearDepth, FarDepth, FieldOfView;
};
