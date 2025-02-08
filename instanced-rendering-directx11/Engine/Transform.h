#pragma once
#include <DirectXMath.h>

class Transform
{
public:
	Transform();

	DirectX::XMFLOAT4X4 GetWorldMatrix();

	DirectX::XMFLOAT4 GetPosition();
	DirectX::XMFLOAT4 GetRotation();
	DirectX::XMFLOAT4 GetScale();

	void SetPosition(DirectX::XMFLOAT3 pos);
	void SetPosition(float x, float y, float z);
	void AddPosition(DirectX::XMFLOAT3 pos);
	void AddPosition(float x, float y, float z);
	void SetRotation(DirectX::XMFLOAT3 rot);
	void SetScale(DirectX::XMFLOAT3 scale);
	
private:
	DirectX::XMFLOAT4 _position;
	DirectX::XMFLOAT4 _rotation;
	DirectX::XMFLOAT4 _scale;
	DirectX::XMFLOAT4X4 _world;
};