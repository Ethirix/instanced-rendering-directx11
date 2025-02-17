#pragma once
#include <DirectXMath.h>

class Camera
{
public:
	void Update(double deltaTime)
	{
		Eye.z += static_cast<float>(deltaTime);

		XMStoreFloat4x4(&View, DirectX::XMMatrixLookToLH(
		XMLoadFloat3(&Eye),
		XMLoadFloat3(&At),
		XMLoadFloat3(&Up)));
	}

	DirectX::XMFLOAT4X4 View, Projection;
	DirectX::XMFLOAT3 Eye, At, Up;

	float NearDepth, FarDepth, FieldOfView;
};
