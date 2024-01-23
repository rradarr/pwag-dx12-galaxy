#pragma once
#include "Mesh.h"

class EngineObject
{
	public:
		int idx;
		Mesh mesh;
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4X4 rotation;
		DirectX::XMFLOAT4X4 worldMat;
		DirectX::XMMATRIX delta_rotXMat;
		DirectX::XMMATRIX delta_rotYMat;
		DirectX::XMMATRIX delta_rotZMat;
		EngineObject() = default;
		EngineObject(int index, Mesh mesh);
	private:
		
};

