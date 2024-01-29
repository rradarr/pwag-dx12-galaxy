#pragma once

#include <string>

using Microsoft::WRL::ComPtr;

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 uvCoordinates;
    DirectX::XMFLOAT3 normal;
};

class Mesh
{
public:
    Mesh() = default;
    Mesh(std::vector<Vertex> vertices, std::vector<DWORD> indices);
    // Load a model (vertices, indices, UVs and vertex colors) from an .obj file.
    void CreateFromFile(const std::string fileName);

    static void CreateCube(std::vector<Vertex>& vertices, std::vector<DWORD>& indices, const int resolution = 2);
    static void CreateSphere(std::vector<Vertex>& vertices, std::vector<DWORD>& indices, const int resolution = 5);

    void InsertDrawIndexed(ComPtr<ID3D12GraphicsCommandList> commandList);
    void InsertBufferBind(ComPtr<ID3D12GraphicsCommandList> commandList);

private:
    bool LoadModelFromFile(const std::string fileName, std::vector<Vertex>& meshVertices, std::vector<DWORD>& meshIndices);
    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // Contains a pointer to the vertex buffer, size of buffer and size of each element.
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    UINT indexCount;
};
