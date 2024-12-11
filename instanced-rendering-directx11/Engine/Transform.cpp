#include "Transform.h"

Transform::Transform()
{
	_position = {};
	_rotation = {};
	_scale = { 1, 1, 1, 0 };
	_world = GetWorldMatrix();
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	if (!_dirty)
		return _world;

	_dirty = false;
	DirectX::XMVECTOR pos = XMLoadFloat4(&_position);
	DirectX::XMVECTOR rot = XMLoadFloat4(&_rotation);
	DirectX::XMVECTOR scale = XMLoadFloat4(&_scale);
	DirectX::XMMATRIX m = DirectX::XMMatrixScalingFromVector(scale)
						* DirectX::XMMatrixRotationRollPitchYawFromVector(rot)
						* DirectX::XMMatrixTranslationFromVector(pos);

	XMStoreFloat4x4(&_world, m);

	return _world;
}

DirectX::XMFLOAT4 Transform::GetPosition()
{
	return _position;
}

DirectX::XMFLOAT4 Transform::GetRotation()
{
	return _rotation;
}

DirectX::XMFLOAT4 Transform::GetScale()
{
	return _scale;
}

void Transform::SetPosition(DirectX::XMFLOAT3 pos)
{
	_position = {pos.x, pos.y, pos.z, 0};
	_dirty = true;
}

void Transform::SetPosition(float x, float y, float z)
{
	_position = {x, y, z, 0};
	_dirty = true;
}

void Transform::AddPosition(DirectX::XMFLOAT3 pos)
{
	_position = {_position.x + pos.x, _position.y + pos.y, _position.z + pos.z, 0};
	_dirty = true;
}

void Transform::AddPosition(float x, float y, float z)
{
	_position = {_position.x + x, _position.y + y, _position.z + z, 0};
	_dirty = true;
}

void Transform::SetRotation(DirectX::XMFLOAT3 rot)
{
	_rotation = {rot.x, rot.y, rot.z, 0};
	_dirty = true;
}

void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	_scale = {scale.x, scale.y, scale.z, 0};
	_dirty = true;
}
