#pragma once
#include <d3d11.h>

struct Mesh
{
	~Mesh()
	{
		RELEASE_RESOURCE(VertexBuffer)
		RELEASE_RESOURCE(IndexBuffer)
	}

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
};
