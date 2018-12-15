#pragma once

#include <d3d11.h>
#include "..\Third party\DirectXMesh-master\DirectXMesh-master\Utilities\WaveFrontReader.h"
#include <wrl/client.h>

namespace DXPanel
{
	class Mesh
	{
		public:
			Mesh() {} // Placeholder constructor for late-initialization with full non-pointer memberss
			Mesh(const wchar_t* meshSrc,
				 const Microsoft::WRL::ComPtr<ID3D11Device>& device);
			~Mesh();
			const Microsoft::WRL::ComPtr<ID3D11Buffer>& VtBuffer();
			const Microsoft::WRL::ComPtr<ID3D11Buffer>& NdxBuffer();
			const DirectX::BoundingBox& BBox();
			const unsigned int NumIndices();
		private:
			WaveFrontReader<unsigned int> wavefront;
			Microsoft::WRL::ComPtr<ID3D11Buffer> vtBuffer;
			Microsoft::WRL::ComPtr<ID3D11Buffer> ndxBuffer;
	};
}
