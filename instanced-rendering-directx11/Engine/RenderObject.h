#pragma once

#include <complex>

#include "GlobalDefs.h"
#include "Mesh.h"
#include "Transform.h"

class RenderObject
{
public:
	void Update(const DirectX::XMFLOAT3& camPos)
	{
		DirectX::XMFLOAT4 pos = Transform.GetPosition();
		DirectX::XMFLOAT3 d = DirectX::XMFLOAT3(camPos.x - pos.x, camPos.y - pos.y, camPos.z - pos.z);
		float mag = std::abs(d.z * d.z);//std::abs(d.x * d.x + d.y * d.y + d.z * d.z);

		Switch = mag < LOD_1 * LOD_1 ? 0 : mag < LOD_2 * LOD_2 ? 1 : 2;
	}

	Transform Transform = {};
	Mesh* LODMeshes[3] = {};
	unsigned Switch = 0;
};