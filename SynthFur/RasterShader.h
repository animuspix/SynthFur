#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace DXPanel
{
	class RasterShader
	{
		public:
			RasterShader() {} // Placeholder constructor for late-initialization with full non-pointer memberss
			RasterShader(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
				LPCWSTR vsFilePath, LPCWSTR psFilePath);
			~RasterShader();
			void Raster(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& deviceContext,
						const Microsoft::WRL::ComPtr<ID3D11Buffer>& projMatInput,
						const Microsoft::WRL::ComPtr<ID3D11Buffer>& settingsInput,
						const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& turingTexReadOnly,
						const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& outputTexRW,
						const UINT meshIndexCount);

		private:
			Microsoft::WRL::ComPtr<ID3D11SamplerState> texSampler;
			Microsoft::WRL::ComPtr<ID3D11VertexShader> vertShader;
			Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
			Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	};
}