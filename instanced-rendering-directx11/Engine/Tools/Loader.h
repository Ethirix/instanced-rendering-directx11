#pragma once
#include <algorithm>
#include <d3d11.h>
#include <DirectXMath.h>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Mesh.h"
#include "../Structs/PerVertexBuffer.h"

struct VertexIndices
{
	unsigned Index;
	unsigned Position;
	unsigned Normal;
};

class Loader
{
public:
	static Mesh* WavefrontOBJLoader(const char* path, ID3D11Device* device)
	{
		Mesh* mesh = new Mesh();

		std::ifstream fileStream;
		std::string currentLine;

		fileStream.open(path);
		if (!fileStream.is_open())
			return {};

		std::vector<DirectX::XMFLOAT3> positions, normals, colours;

		std::unordered_map<std::string, VertexIndices> faces{};
		std::vector<UINT> indices;
		UINT currentIndex = 0;

		while (std::getline(fileStream, currentLine))
		{
			std::istringstream stringStream(currentLine);
			std::string objToken;
			stringStream >> objToken;

			if (objToken == "v")
			{
				DirectX::XMFLOAT3 v3{};
				stringStream >> v3.x;
				stringStream >> v3.y;
				stringStream >> v3.z;
				positions.emplace_back(v3);
				stringStream >> v3.x;
				stringStream >> v3.y;
				stringStream >> v3.z;
				colours.emplace_back(v3);
			}
			else if (objToken == "vn")
			{
				DirectX::XMFLOAT3 v3{};
				stringStream >> v3.x;
				stringStream >> v3.y;
				stringStream >> v3.z;
				normals.emplace_back(v3);
			}
			else if (objToken == "f")
			{
				VertexIndices currentFace{};
				USHORT polygonSideCount = 0;

				while (true)
				{
					std::string indicesLine;
					stringStream >> indicesLine;

					if (indicesLine.empty())
						break;

					if (polygonSideCount++ > 3)
						return {};

					if (faces.contains(indicesLine))
					{
						indices.emplace_back(faces.find(indicesLine)->second.Index);
						continue;
					}

					std::string replacedLine = indicesLine;
					std::ranges::replace(replacedLine, '/', ' ');
					std::istringstream indicesStringStream(replacedLine);

					UINT index;
					indicesStringStream >> index;
					currentFace.Position = index - 1;
					indicesStringStream >> index;
					indicesStringStream >> index;
					currentFace.Normal = index - 1;
					currentFace.Index = currentIndex++;

					faces.try_emplace(indicesLine, currentFace);
					indices.emplace_back(currentFace.Index);
				}
			}
		}

		std::vector<VertexIndices> orderedFaces;
		for (int i = 0; i < faces.size(); ++i)
		{
			for (VertexIndices& vertex : faces | std::views::values)
			{
				if (vertex.Index == i)
				{
					orderedFaces.emplace_back(vertex);
					break;
				}
			}
		}

		PerVertexBuffer* vertices = new PerVertexBuffer[faces.size()];
		size_t verticesCount = faces.size();
		{
			unsigned i = 0;
			for (const VertexIndices& vertex : orderedFaces)
			{
				vertices[i].Position = positions[vertex.Position];
				vertices[i].Normal = normals[vertex.Normal];
				vertices[i].Colour = colours[vertex.Position];

				i++;
			}
		}
		UINT* index = new UINT[indices.size()];
		size_t indexCount = indices.size();
		for (unsigned i = 0; i < indices.size(); ++i)
			index[i] = indices[i];

		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.ByteWidth = sizeof(PerVertexBuffer) * verticesCount;
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA subresourceVertexData = { vertices };
		//delete [] vertices;

		HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &subresourceVertexData, &mesh->VertexBuffer);
		assert(SUCCEEDED(hr));

		D3D11_BUFFER_DESC indexBufferDesc = {};
		indexBufferDesc.ByteWidth = sizeof(UINT) * indexCount;
		indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA subresourceIndexData = { index };
		//delete [] index;

		hr = device->CreateBuffer(&indexBufferDesc, &subresourceIndexData, &mesh->IndexBuffer);
		assert(SUCCEEDED(hr));

		return mesh;
	}
};