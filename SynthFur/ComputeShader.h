#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <assert.h>
#include <windows.h>
#include <wrl\client.h>
#include "pch.h"

namespace DXPanel
{
	struct ComputeShader
	{
		ComputeShader() {} // Placeholder constructor for late-initialization with full non-pointer memberss
		ComputeShader(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
			LPCWSTR shaderFilePath)
		{
			// Declare a buffer to store the imported shader data
			Microsoft::WRL::ComPtr<ID3DBlob> shaderBuffer;

			// Import the given file into DirectX with [D3DReadFileToBlob(...)]
			HRESULT result = D3DReadFileToBlob(shaderFilePath, &shaderBuffer);
			assert(SUCCEEDED(result));

			// Construct the shader object from the buffer populated above
			result = device->CreateComputeShader(shaderBuffer->GetBufferPointer(),
				shaderBuffer->GetBufferSize(),
				NULL,
				&shader);
			assert(SUCCEEDED(result));
		}
		~ComputeShader()
		{
			shader = nullptr;
		}

		// The core shader object associated with [this]
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader;
	};
}