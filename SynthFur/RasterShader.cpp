#include "pch.h"
#include "RasterShader.h"

DXPanel::RasterShader::RasterShader(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
									LPCWSTR vsFilePath, LPCWSTR psFilePath)
{
	// Read the vertex shader into a buffer on the GPU
	Microsoft::WRL::ComPtr<ID3DBlob> vertShaderBuffer;
	D3DReadFileToBlob(vsFilePath, &vertShaderBuffer);

	// Read the pixel shader into a buffer on the GPU
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBuffer;
	D3DReadFileToBlob(psFilePath, &pixelShaderBuffer);

	// Create the vertex shader from the vertex shader buffer
	HRESULT result = device->CreateVertexShader(vertShaderBuffer->GetBufferPointer(), vertShaderBuffer->GetBufferSize(), NULL, &vertShader);
	if (FAILED(result)) { throw std::runtime_error::runtime_error("DX11 state error"); } // No assertions available here because of definitions in [pch.h], using exceptions instead

	// Create the pixel shader from the pixel shader buffer
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader);
	if (FAILED(result)) { throw std::runtime_error::runtime_error("DX11 state error"); }

	// Describe the vertex input layout
	D3D11_INPUT_ELEMENT_DESC inputInfo[3];
	inputInfo[0].SemanticName = "POSITION";
	inputInfo[0].SemanticIndex = 0;
	inputInfo[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputInfo[0].InputSlot = 0;
	inputInfo[0].AlignedByteOffset = 0;
	inputInfo[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputInfo[0].InstanceDataStepRate = 0;
	inputInfo[1].SemanticName = "NORMAL";
	inputInfo[1].SemanticIndex = 0;
	inputInfo[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputInfo[1].InputSlot = 0;
	inputInfo[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	inputInfo[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputInfo[1].InstanceDataStepRate = 0;
	inputInfo[2].SemanticName = "TEXCOORD";
	inputInfo[2].SemanticIndex = 0;
	inputInfo[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputInfo[2].InputSlot = 0;
	inputInfo[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	inputInfo[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputInfo[2].InstanceDataStepRate = 0;

	// Create the vertex input layout
	result = device->CreateInputLayout(inputInfo, 3, vertShaderBuffer->GetBufferPointer(),
									   vertShaderBuffer->GetBufferSize(), &inputLayout);
	if (FAILED(result)) { throw std::runtime_error::runtime_error("DX11 state error"); }

	// Describe + create the texture sampler
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 8;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 0;
	result = device->CreateSamplerState(&samplerDesc, &texSampler);
	if (FAILED(result)) { throw std::runtime_error::runtime_error("DX11 state error"); }
}

DXPanel::RasterShader::~RasterShader()
{
	vertShader = nullptr;
	pixelShader = nullptr;
	inputLayout = nullptr;
}

void DXPanel::RasterShader::Raster(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& deviceContext,
								   const Microsoft::WRL::ComPtr<ID3D11Buffer>& projMatInput,
								   const Microsoft::WRL::ComPtr<ID3D11Buffer>& settingsInput,
								   const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& turingTexReadOnly,
								   const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& outputTexRW,
								   const UINT meshIndexCount)
{
	// Initialise the pixel shader's texture sampler state with [wrapSamplerState]
	deviceContext->PSSetSamplers(0, 1, texSampler.GetAddressOf());
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(inputLayout.Get());
	// Set the vertex/pixel shaders that will be used to render
	// the screen rect
	deviceContext->VSSetShader(vertShader.Get(), NULL, 0);
	deviceContext->PSSetShader(pixelShader.Get(), NULL, 0);
	// Pass the given input buffers along to the GPU
	deviceContext->VSSetConstantBuffers(0, 1, projMatInput.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, settingsInput.GetAddressOf());
	// Pass the shader-resource version of the display texture along to the GPU
	deviceContext->PSSetShaderResources(0, 1, turingTexReadOnly.GetAddressOf());
	deviceContext->CSSetUnorderedAccessViews(1, 1, outputTexRW.GetAddressOf(), 0);
	// Render the screen rect
	deviceContext->DrawIndexed(meshIndexCount, 0, 0);
	// Resources can't be exposed through unordered-access-views (read/write allowed) and shader-
	// resource-views (read-only) simultaneously, so un-bind the local shader-resource-view
	// of the Turing texture over here
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> nullSRV = nullptr;
	deviceContext->PSSetShaderResources(0, 1, &nullSRV);
}
