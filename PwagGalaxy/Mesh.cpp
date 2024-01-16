#include "stdafx.h"
#include "Mesh.h"

#include "dx_includes/DXSampleHelper.h"
#include "EngineHelpers.h"
#include "BufferMemoryManager.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<DWORD> indices)
{
    BufferMemoryManager buffMng;

    indexCount = indices.size();

    UINT vertexBufferSize = vertices.size() * sizeof(Vertex);
    ComPtr<ID3D12Resource> vertexUploadBuffer;
    buffMng.AllocateBuffer(vertexUploadBuffer, vertexBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    buffMng.AllocateBuffer(vertexBuffer, vertexBufferSize, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT);

    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<BYTE*>(vertices.data());
    vertexData.RowPitch = vertexBufferSize;
    vertexData.SlicePitch = vertexBufferSize;

    buffMng.FillBuffer(vertexBuffer, vertexData, vertexUploadBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // Initialize the vertex buffer view.
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = vertexBufferSize;

    UINT indexBufferSize = indices.size() * sizeof(DWORD);
    ComPtr<ID3D12Resource> indexUploadBuffer;
    buffMng.AllocateBuffer(indexUploadBuffer, indexBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    buffMng.AllocateBuffer(indexBuffer, indexBufferSize, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT);

    D3D12_SUBRESOURCE_DATA indexData = {};
    indexData.pData = reinterpret_cast<BYTE*>(indices.data());
    indexData.RowPitch = indexBufferSize;
    indexData.SlicePitch = indexBufferSize;

    buffMng.FillBuffer(indexBuffer, indexData, indexUploadBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    // Initialize the index buffer view.
    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView.SizeInBytes = indexBufferSize;
}

void Mesh::CreateFromFile(const std::string fileName)
{
    BufferMemoryManager buffMng;

    std::vector<Vertex> triangleVertices;
    std::vector<DWORD> triangleIndices;
    LoadModelFromFile(fileName, triangleVertices, triangleIndices);
    indexCount = triangleIndices.size();

    UINT vertexBufferSize = triangleVertices.size() * sizeof(Vertex);
    ComPtr<ID3D12Resource> vertexUploadBuffer;
    buffMng.AllocateBuffer(vertexUploadBuffer, vertexBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    buffMng.AllocateBuffer(vertexBuffer, vertexBufferSize, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT);

    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<BYTE*>(triangleVertices.data());
    vertexData.RowPitch = vertexBufferSize;
    vertexData.SlicePitch = vertexBufferSize;

    // We don't close the commandList and wait for it to finish as we have more commands to upload.
    buffMng.FillBuffer(vertexBuffer, vertexData, vertexUploadBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // Initialize the vertex buffer view.
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = vertexBufferSize;

    UINT indexBufferSize = triangleIndices.size() * sizeof(DWORD);
    ComPtr<ID3D12Resource> indexUploadBuffer;
    buffMng.AllocateBuffer(indexUploadBuffer, indexBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    buffMng.AllocateBuffer(indexBuffer, indexBufferSize, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT);

    D3D12_SUBRESOURCE_DATA indexData = {};
    indexData.pData = reinterpret_cast<BYTE*>(triangleIndices.data());
    indexData.RowPitch = indexBufferSize;
    indexData.SlicePitch = indexBufferSize;

    buffMng.FillBuffer(indexBuffer, indexData, indexUploadBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    // Initialize the index buffer view.
    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView.SizeInBytes = indexBufferSize;
}

void Mesh::InsertDrawIndexed(ComPtr<ID3D12GraphicsCommandList> commandList)
{
    // Should the root descriptor he here too? -> possibly should be level up, in some DrawableObject class
    //commandList->SetGraphicsRootConstantBufferView(1, m_constantRootDescriptorBuffers[m_frameBufferIndex]->GetGPUVirtualAddress());
    commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void Mesh::InsertBufferBind(ComPtr<ID3D12GraphicsCommandList> commandList)
{
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);
}

bool Mesh::LoadModelFromFile(const std::string fileName, std::vector<Vertex>& meshVertices, std::vector<DWORD>& meshIndices)
{
    std::wstring tmpName(fileName.begin(), fileName.end());
    LPCWSTR wideFileName = tmpName.c_str();

    std::wstring inputFileWide = { EngineHelpers::GetAssetFullPath(wideFileName).c_str() };
    std::string inputFile = { inputFileWide.begin(), inputFileWide.end() };
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputFile.c_str());

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        std::cout << "Model did not load!" << std::endl;
        return false;
    }

    // Vertices are stored in a vector as x, y, z floats, so vector size is divided by 3.
    meshVertices.resize(attrib.vertices.size() / 3);

    // Each shape can be thought of as a separate mesh in the file, so a sum of index
    // counts for all shapes is needed.
    unsigned int numOfIndices = 0;
    for (int shapeID = 0; shapeID < shapes.size(); shapeID++)
    {
        numOfIndices += shapes[shapeID].mesh.indices.size();
    }
    meshIndices.resize(numOfIndices);

    // Iterate over shapes in the file.
    for (int shapeID = 0; shapeID < shapes.size(); shapeID++)
    {
        // Iterate over indices in the shape. They index into attrib.vertices as if
        // each xyz triple was one element (so max index will be attrib.vertices.size / 3)
        int shapeIndexNumber = shapes[shapeID].mesh.indices.size();
        for (int vertexID = 0; vertexID < shapeIndexNumber; vertexID++) {
            meshIndices[shapeID * shapeIndexNumber + vertexID] = static_cast<DWORD>(shapes[shapeID].mesh.indices[vertexID].vertex_index);

            // Store verte data in put structure.
            float vertexX, vertexY, vertexZ, vertexU, vertexV;
            vertexX = attrib.vertices[3 * shapes[shapeID].mesh.indices[vertexID].vertex_index];
            vertexY = attrib.vertices[3 * shapes[shapeID].mesh.indices[vertexID].vertex_index + 1];
            vertexZ = attrib.vertices[3 * shapes[shapeID].mesh.indices[vertexID].vertex_index + 2];
            vertexU = attrib.texcoords[2 * shapes[shapeID].mesh.indices[vertexID].texcoord_index];
            vertexV = attrib.texcoords[2 * shapes[shapeID].mesh.indices[vertexID].texcoord_index + 1];
            Vertex vertex = {
                {vertexX, vertexY, vertexZ},
                {1.f, 1.f, 1.f, 1.f},
                {vertexU, vertexV}
            };
            meshVertices[meshIndices[shapeID * 3 + vertexID]] = vertex;
        }
    }
}
