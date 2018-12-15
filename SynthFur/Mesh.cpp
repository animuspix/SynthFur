#include "pch.h"
#include "Mesh.h"

DXPanel::Mesh::Mesh(const wchar_t* meshSrc,
					const Microsoft::WRL::ComPtr<ID3D11Device>& device)
{
	// Load vertices from [meshSrc]
	HRESULT result = wavefront.Load(meshSrc);
	assert(SUCCEEDED(result));

	// Describe the vertex buffer
	D3D11_BUFFER_DESC vertBufferDesc;
	vertBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertBufferDesc.ByteWidth = (UINT)(sizeof(decltype(wavefront.vertices[0])) * wavefront.vertices.size());
	vertBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertBufferDesc.CPUAccessFlags = 0;
	vertBufferDesc.MiscFlags = 0;
	vertBufferDesc.StructureByteStride = 0;

	// Create vertex buffer
	D3D11_SUBRESOURCE_DATA vertData;
	vertData.pSysMem = &(wavefront.vertices[0]);
	vertData.SysMemPitch = 0;
	vertData.SysMemSlicePitch = 0;
	result = device->CreateBuffer(&vertBufferDesc, &vertData, &vtBuffer);
	assert(SUCCEEDED(result));

	// Describe + create the index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = (UINT)(sizeof(decltype(wavefront.indices[0])) * wavefront.indices.size());
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = &wavefront.indices[0];
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &ndxBuffer);
	assert(SUCCEEDED(result));
}

DXPanel::Mesh::~Mesh()
{
	vtBuffer = nullptr;
	ndxBuffer = nullptr;
}

const Microsoft::WRL::ComPtr<ID3D11Buffer>& DXPanel::Mesh::VtBuffer()
{
	return vtBuffer;
}

const Microsoft::WRL::ComPtr<ID3D11Buffer>& DXPanel::Mesh::NdxBuffer()
{
	return ndxBuffer;
}

const DirectX::BoundingBox& DXPanel::Mesh::BBox()
{
	return wavefront.bounds;
}

const unsigned int DXPanel::Mesh::NumIndices()
{
	return (unsigned int)(wavefront.indices.size());
}
