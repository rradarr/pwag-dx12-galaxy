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

void Mesh::CreateCube(std::vector<Vertex>& vertices, std::vector<DWORD>& indices, const int resolution)
{
    assert(resolution > 1);

    int vertexRowLength = (resolution - 1) * 4;

    int sideVertexTotal = resolution * (resolution - 1);
    int topBottomVertexTotal = (resolution - 2) * (resolution - 2);
    int vertexTotal = ((sideVertexTotal * 4) + (2 * topBottomVertexTotal));
    vertices.reserve(vertexTotal);

    Vertex vert;
    vert.color = DirectX::XMFLOAT4(1, 1, 1, 1);
    vert.uvCoordinates = { 0.f, 0.f };
    vert.normal = { 0.f, 0.f, 0.f };

    // Generate the cube.

    // Generate cube walls as one long vertex list (a plane that can be folded into the walls of a cube).
    DirectX::XMVECTOR sideFaceNormals[] = {
        DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),   // front
        DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f),  // right
        DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f),  // back
        DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),   // left
    };
    DirectX::XMVECTOR globalUp = { 0.f, 1.f, 0.f, 0.f };
    // Generate each wall with one less column of vertices at the edge.
    for (int y = 0; y < resolution; y++)
    {
        for (int face = 0; face < 4; face++)
        {
            DirectX::XMVECTOR sideNormal = sideFaceNormals[face];
            DirectX::XMVECTOR yAxis = globalUp;
            DirectX::XMVECTOR xAxis = DirectX::XMVector3Cross(yAxis, sideNormal);

            for (int x = 0; x < resolution - 1; x++)
            {
                float xPercent = (float)x / (resolution - 1);
                float yPercent = (float)y / (resolution - 1);
                float xStage = (xPercent - 0.5f) * 2.0f;
                float yStage = (yPercent - 0.5f) * 2.0f;

                DirectX::XMVECTOR px = DirectX::XMVectorScale(xAxis, xStage);
                DirectX::XMVECTOR py = DirectX::XMVectorScale(yAxis, yStage);
                DirectX::XMVECTOR point = DirectX::XMVectorAdd(sideNormal, px);
                point = DirectX::XMVectorAdd(point, py);
                //point = DirectX::XMVector3Normalize(point);
                DirectX::XMStoreFloat3(&vert.position, point);

                vertices.push_back(vert);
            }
        }
    }

    // Generate cube top and bootom (only inner/non-edge vertices).
    DirectX::XMVECTOR TopBottomSideNormals[] = {
        DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f),   // top
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),  // bottom
    };
    DirectX::XMVECTOR globalRight = { 1.f, 0.f, 0.f, 0.f };
    for (int face = 0; face < 2; face++)
    {
        DirectX::XMVECTOR sideNormal = TopBottomSideNormals[face];
        DirectX::XMVECTOR xAxis = DirectX::XMVectorNegate(DirectX::XMVector3Cross(sideNormal, globalRight));
        if (face == 1)
            xAxis = DirectX::XMVectorNegate(xAxis);
        DirectX::XMVECTOR yAxis = DirectX::XMVectorNegate(globalRight);

        // Generate only internal vertices (no top and bottom row, and no left and right column).
        for (int y = 1; y < resolution - 1; y++)
        {
            for (int x = 1; x < resolution - 1; x++)
            {
                float xPercent = (float)x / (resolution - 1);
                float yPercent = (float)y / (resolution - 1);
                float xStage = (xPercent - 0.5f) * 2.0f;
                float yStage = (yPercent - 0.5f) * 2.0f;

                DirectX::XMVECTOR px = DirectX::XMVectorScale(xAxis, xStage);
                DirectX::XMVECTOR py = DirectX::XMVectorScale(yAxis, yStage);
                DirectX::XMVECTOR point = DirectX::XMVectorAdd(sideNormal, px);
                point = DirectX::XMVectorAdd(point, py);
                //point = DirectX::XMVector3Normalize(point);
                DirectX::XMStoreFloat3(&vert.position, point);

                vertices.push_back(vert);
            }
        }
    }

    // Generate the indices.
    int indexTotal = 36 * (resolution - 1) * (resolution - 1); // (6 faces * squares per face * 2 tris per square * 3 indices per tris)
    indices.reserve(indexTotal);

    // Generate indices for sides.
    for (int y = 0; y < resolution - 1; y++) {
        for (int x = 0; x < vertexRowLength - 1; x++) {
            int vertexId = x + y * vertexRowLength;

            indices.push_back(vertexId);
            indices.push_back(vertexId + 1);
            indices.push_back(vertexId + vertexRowLength + 1);

            indices.push_back(vertexId);
            indices.push_back(vertexId + vertexRowLength + 1);
            indices.push_back(vertexId + vertexRowLength);
        }
    }
    // Connect sides at last edge.
    for (int y = 0; y < resolution - 1; y++) {
        int vertexId = (vertexRowLength - 1) + y * vertexRowLength;

        indices.push_back(vertexId);
        indices.push_back(y * vertexRowLength);
        indices.push_back((y + 1) * vertexRowLength);

        indices.push_back(vertexId);
        indices.push_back((y + 1) * vertexRowLength);
        indices.push_back(vertexId + vertexRowLength);
    }

    // Create pseudo sides for top and bottom, for easier triangulation.
    std::vector<int> topSideHelper;
    topSideHelper.reserve(resolution * resolution);
    for (int i = 0; i < resolution; i++) {
        topSideHelper.push_back(i); // indices of 'top' edge.
    }
    // fill the middle space with vertex indices.
    for (int y = 0; y < resolution - 2; y++) {
        topSideHelper.push_back(4 * (resolution - 1) - 1 - y); // left vertex of this row
        for (int x = 0; x < resolution - 2; x++) {
            topSideHelper.push_back(4 * sideVertexTotal + x + y * (resolution - 2));
        }
        topSideHelper.push_back(resolution + y); // right vertex of this row
    }
    for (int i = 3 * (resolution - 1); i >= 2 * (resolution - 1); i--) {
        topSideHelper.push_back(i); // indices of 'bottom' edge.
    }

    std::vector<int> bottomSideHelper;
    bottomSideHelper.reserve(resolution * resolution);
    for (int i = 0; i < resolution; i++) {
        bottomSideHelper.push_back(i + (resolution - 1) * vertexRowLength); // indices of 'top' edge.
    }
    // fill the middle space with vertex indices.
    for (int y = 0; y < resolution - 2; y++) {
        bottomSideHelper.push_back(4 * (resolution - 1) - 1 - y + (resolution - 1) * vertexRowLength); // left vertex of this row
        for (int x = 0; x < resolution - 2; x++) {
            bottomSideHelper.push_back(4 * sideVertexTotal + topBottomVertexTotal + x + y * (resolution - 2));
        }
        bottomSideHelper.push_back(resolution + y + (resolution - 1) * vertexRowLength); // right vertex of this row
    }
    for (int i = 3 * (resolution - 1) + (resolution - 1) * vertexRowLength; i >= 2 * (resolution - 1) + (resolution - 1) * vertexRowLength; i--) {
        bottomSideHelper.push_back(i); // indices of 'bottom' edge.
    }

    // Generate indices for top and bottom using the heplers.
    for (int y = 0; y < resolution - 1; y++) {
        for (int x = 0; x < resolution - 1; x++) {
            int helperIdPos = x + y * resolution; // Position of the index we want in the hepler vertex.
            indices.push_back(topSideHelper[helperIdPos]);
            indices.push_back(topSideHelper[helperIdPos + resolution + 1]);
            indices.push_back(topSideHelper[helperIdPos + 1]);

            indices.push_back(topSideHelper[helperIdPos + resolution + 1]);
            indices.push_back(topSideHelper[helperIdPos]);
            indices.push_back(topSideHelper[helperIdPos + resolution]);
        }
    }
    for (int y = 0; y < resolution - 1; y++) {
        for (int x = 0; x < resolution - 1; x++) {
            int helperIdPos = x + y * resolution; // Position of the index we want in the hepler vertex.
            indices.push_back(bottomSideHelper[helperIdPos]);
            indices.push_back(bottomSideHelper[helperIdPos + 1]);
            indices.push_back(bottomSideHelper[helperIdPos + resolution + 1]);

            indices.push_back(bottomSideHelper[helperIdPos]);
            indices.push_back(bottomSideHelper[helperIdPos + resolution + 1]);
            indices.push_back(bottomSideHelper[helperIdPos + resolution]);
        }
    }
}

void Mesh::CreateSphere(std::vector<Vertex>& vertices, std::vector<DWORD>& indices, const int resolution)
{
    CreateCube(vertices, indices, resolution);
    for (int i = 0; i < vertices.size(); i++) {
        DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&vertices[i].position);
        position = DirectX::XMVector3Normalize(position);
        DirectX::XMStoreFloat3(&vertices[i].position, position);
    }
}

void Mesh::InsertDrawIndexed(ComPtr<ID3D12GraphicsCommandList> commandList)
{
    // Should the root descriptor he here too? -> possibly should be level up, in some DrawableObject class
    //commandList->SetGraphicsRootConstantBufferView(1, m_WVPConstantBuffers[m_frameBufferIndex]->GetGPUVirtualAddress());
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
            float vertexX, vertexY, vertexZ, vertexU, vertexV, normalX, normalY, normalZ;

            vertexX = attrib.vertices[3 * shapes[shapeID].mesh.indices[vertexID].vertex_index];
            vertexY = attrib.vertices[3 * shapes[shapeID].mesh.indices[vertexID].vertex_index + 1];
            vertexZ = attrib.vertices[3 * shapes[shapeID].mesh.indices[vertexID].vertex_index + 2];

            vertexU = attrib.texcoords[2 * shapes[shapeID].mesh.indices[vertexID].texcoord_index];
            vertexV = attrib.texcoords[2 * shapes[shapeID].mesh.indices[vertexID].texcoord_index + 1];

            normalX = attrib.normals[3 * shapes[shapeID].mesh.indices[vertexID].normal_index];
            normalY = attrib.normals[3 * shapes[shapeID].mesh.indices[vertexID].normal_index + 1];
            normalZ = attrib.normals[3 * shapes[shapeID].mesh.indices[vertexID].normal_index + 2];

            Vertex vertex = {
                {vertexX, vertexY, vertexZ},
                {1.f, 1.f, 1.f, 1.f},
                {vertexU, vertexV},
                {normalX, normalY, normalZ}
            };
            meshVertices[meshIndices[shapeID * 3 + vertexID]] = vertex;
        }
    }

    return true;
}
