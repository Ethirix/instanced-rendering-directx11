#pragma once
#include <d3d11.h>

#include "GlobalDefs.h"
#include "Structs/PerVertexBuffer.h"

struct Mesh
{
	~Mesh()
	{
		RELEASE_RESOURCE(VertexBuffer)
		RELEASE_RESOURCE(IndexBuffer)

		delete [] Vertices;
		delete [] Indices;
	}

	PerVertexBuffer* Vertices;
	UINT* Indices;

	size_t VerticesCount;
	size_t IndicesCount;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
};
