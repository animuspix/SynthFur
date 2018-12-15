#include "pch.h"
#include "SynthFurDXPanel.h"
#include "SimSettings.h"
#include "RasterShader.h"
#include "ComputeShader.h"
#include "Mesh.h"
#include <windows.ui.xaml.media.dxinterop.h>
#include <windows.graphics.directx.direct3d11.interop.h>

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
//using namespace Windows::Graphics::Imaging;
using namespace Windows::System::Threading;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input::Inking;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Interop;
using namespace Concurrency;

DXPanel::SynthFurDXPanel::SynthFurDXPanel()
{
	// Setup event handlers for [this]
	this->SizeChanged += ref new SizeChangedEventHandler(this, &DXPanel::SynthFurDXPanel::OnSizeChanged);
	this->CompositionScaleChanged += ref new Windows::Foundation::TypedEventHandler<SwapChainPanel^, Object^>(this, &DXPanel::SynthFurDXPanel::OnCompositionScaleChanged);
	Application::Current->Suspending += ref new SuspendingEventHandler(this, &DXPanel::SynthFurDXPanel::OnSuspending);
	Application::Current->Resuming += ref new EventHandler<Object^>(this, &DXPanel::SynthFurDXPanel::OnResuming);
	initted = false; // Full initialization won't occur until the user loads a mesh
}

DXPanel::SynthFurDXPanel::~SynthFurDXPanel()
{
	// End the simulation loop
	if (initted) { simLoopWorker->Cancel(); }
	// Clear heap-allocated objects
	delete mesh;
	mesh = nullptr;
	delete texGen;
	texGen = nullptr;
	delete meshPresentation;
	meshPresentation = nullptr;
	delete simSettings;
	simSettings = nullptr;
}

void DXPanel::SynthFurDXPanel::InitDX()
{
	// Target Direct3D 11.1
	D3D_FEATURE_LEVEL featureLvl = D3D_FEATURE_LEVEL_11_1;
	// Enable debug output if compiling in debug mode
	HRESULT result = D3D11CreateDevice(nullptr,
								       D3D_DRIVER_TYPE_HARDWARE,
								       NULL,
									   #ifdef _DEBUG
								     		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
									   #else
								     		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
								       #endif
								       &featureLvl,
								       1,
								       D3D11_SDK_VERSION,
								       &device,
								       nullptr,
								       &deviceContext);
	assert(SUCCEEDED(result));

	// Clear device information from previous initializations
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	deviceContext->Flush();

	// Construct the swapchain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = (UINT)ActualWidth;      // Match the size of the panel.
	swapChainDesc.Height = (UINT)ActualHeight;
	swapChainDesc.Format = SWAP_CHAIN_FORMAT; // Use the standard UWP color order (BGRA/ARGB)
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;                                 // Avoid performance hit from multisampling
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 3;                                      // Triple buffering
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;        // All Windows Store apps must use this SwapEffect.
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // Ignore alpha
	swapChainDesc.Flags = 0;
	Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
	result = device.As(&dxgiDevice); // Get low-level DXGI device
	assert(SUCCEEDED(result));
	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter; // Get a reference to the local display adapter
	result = dxgiDevice->GetAdapter(&dxgiAdapter);
	assert(SUCCEEDED(result));
	Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
	result = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)); // Obtain a specialized factory for swap-chains displayed within
																 // UWP apps
	result = dxgiFactory->CreateSwapChainForComposition(device.Get(),
														&swapChainDesc,
														nullptr,
														&swapChain); // Create the swapchain
	// Asynchronously associate the generated swap-chain with the swap-chain panel
	Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([=]()
	{
		// Extract the native swap-chain interface held by [SwapChainPanel]
		Microsoft::WRL::ComPtr<ISwapChainPanelNative> panelNative;
		HRESULT asResult = ((IUnknown*)(this))->QueryInterface(IID_PPV_ARGS(&panelNative));
		assert(SUCCEEDED(result));

		// Associate the swap-chain we created before with the swap-chain panel; this must be done on the UI thread
		asResult = panelNative->SetSwapChain(swapChain.Get());
		assert(SUCCEEDED(result));
	}, CallbackContext::Any));

	// Create a render-target view for rasterization
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	result = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &defaultRenderTarget);
	assert(SUCCEEDED(result));

	// Provide a viewport (for rasterization, again)
	D3D11_VIEWPORT vp = CD3D11_VIEWPORT(backBuffer.Get(), defaultRenderTarget.Get());
	deviceContext->RSSetViewports(1, &vp); // No error code on viewport definition
	assert(SUCCEEDED(result));

	// Describe, create, assign rasterizer state
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = true;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = false;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = true;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState;
	result = device->CreateRasterizerState(&rasterDesc, rasterState.GetAddressOf());
	assert(SUCCEEDED(result));
	deviceContext->RSSetState(rasterState.Get());

	// Describe, create, assign depth/stencil state
	// Stencilling operations are left as default
	D3D11_DEPTH_STENCIL_DESC dStencilDesc;
	dStencilDesc.DepthEnable = false;
	dStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dStencilDesc.StencilEnable = FALSE;
	dStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	dStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> dsState;
	result = device->CreateDepthStencilState(&dStencilDesc, dsState.GetAddressOf());
	assert(SUCCEEDED(result));
	deviceContext->OMSetDepthStencilState(dsState.Get(), 0);

	// Describe + create depth/stencil buffer
	CD3D11_TEXTURE2D_DESC dsDesc(DXGI_FORMAT_D24_UNORM_S8_UINT,
								 (UINT)ActualWidth, // Swap-chain width
								 (UINT)ActualHeight, // Swap-chain height
								 1, // We only want to create one depth/stencil buffer
								 1, // One level of mipmapping (since we're just rendering a static rotating model)
								 D3D11_BIND_DEPTH_STENCIL); // Bind to the depth and stencil buffers simultaneously
	Microsoft::WRL::ComPtr<ID3D11Texture2D> dsBuf;
	result = device->CreateTexture2D(&dsDesc, nullptr, &dsBuf);
	assert(SUCCEEDED(result));
	// Describe + create a view over the depth/stencil buffer
	CD3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D);
	result = device->CreateDepthStencilView(dsBuf.Get(),
											&dsViewDesc,
											&depthStencil);
	assert(SUCCEEDED(result));
	// Pass depth stencil + render target to the GPU
	deviceContext->OMSetRenderTargets(1, defaultRenderTarget.GetAddressOf(), depthStencil.Get());
	// Create perspective projection matrix
	perspProj = DirectX::XMMatrixPerspectiveFovLH(1.62f,
												  (float)(ActualWidth / ActualHeight),
												  0.001f,
												  200.0f);
	// Create view projection matrix
	camPos = DirectX::XMVectorSet(0.0f, 0.0f, -2.0f, 0.0f);
	viewMat = DirectX::XMMatrixLookAtLH(camPos,
										DirectX::XMVectorSet(0.0f, 0.01f, 0.0f, 0.0f),
										DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	// Different matrix ordering on cpu/gpu
	viewMat = DirectX::XMMatrixTranspose(viewMat);
	perspProj = DirectX::XMMatrixTranspose(perspProj);
	// Construct the simulation clock, initialize recent timepoint
	simClock = std::chrono::steady_clock();
	lastFrameTime = simClock.now();
}

void DXPanel::SynthFurDXPanel::InitTuringSettings()
{
	// Initialize Turing-specific properties to the values given in:
	// http://www.karlsims.com/rd.html
	SetTuringDiffusionU(1.0f);
	SetTuringDiffusionV(0.5f);
	SetTuringConvRate(0.062f);
	SetTuringFeedRate(0.055f);
	SetTuringKrnCenter(-1);
	SetTuringKrnElt(200, 0);
	SetTuringKrnElt(200, 1);
	SetTuringKrnElt(200, 2);
	SetTuringKrnElt(200, 3);
	SetTuringKrn2Elt(50, 0);
	SetTuringKrn2Elt(50, 1);
	SetTuringKrn2Elt(50, 2);
	SetTuringKrn2Elt(50, 3);
}

void DXPanel::SynthFurDXPanel::InitSettings()
{
	// Initialize generic shading properties
	SetBaseFadeState(0);
	SetBaseShadingAxis(1);
	SetTuringShadingAxis(1);
	SetTuringBlendOp(0);
	SetBaseFadeRate(1.0f);
	SetTuringFadeRate(1.0f);
	SetLiIntensity(1.0f);
	camZoom = -2.0f;
	camOrbi = 0.0f;
	simSettings->minCoordLiTheta.w = 0.0f;
	SetTuringProjection(0); // Default to user UV coordinates for pattern projection
	simSettings->turingCenter.z = 0; // Pattern reset is off by default
	surfSpinning = true; // Enable surface spinning by default
	simSettings->fadingBase.w = 1; // Clamp synthesized colors by default
	InitTuringSettings(); // Initialize settings for Turing patterns; in a helper
						  // method for easier access on reaction/diffusion setting
						  // reset
	// Pattern simulation is un-paused by default
	patternSimPaused = false;
}

void DXPanel::SynthFurDXPanel::IterTexSim()
{
	// Measure current time
	std::chrono::time_point t = simClock.now();
	// Evaluate delta-time, convert from nanoseconds rational to
	// seconds floating-point
	float dt = ((float)(t - lastFrameTime).count()) / 1000000000;
	// Convert nanosecond delta-time to one-second floating-point
	simSettings->timeMaxCoord.x = dt;
	// Update light/camera rotation angle so light direction matches camera
	// orientation at draw-time (irrelevant to texture simulation, but included
	// here anyway so we can keep singly-updating the input buffer)
	simSettings->minCoordLiTheta.w += (surfSpinning) ? dt : 0.0f;
	// Update last-frame time
	lastFrameTime = simClock.now();
	if (!gpuPaused)
	{
		// Update simulation settings, pass them along to the GPU as well
		D3D11_MAPPED_SUBRESOURCE input;
		HRESULT result = deviceContext->Map(simInput.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &input);
		assert(SUCCEEDED(result));
		SimSettings* settingsPttr = (SimSettings*)input.pData;
		memcpy(settingsPttr, simSettings, sizeof(SimSettings)); // Memcpy update because we can and it's *fast*
		deviceContext->Unmap(simInput.Get(), 0);
		if (!patternSimPaused)
		{
			// Pass iteration shader onto the GPU
			deviceContext->CSSetShader(texGen->shader.Get(), nullptr, 0);
			// Pass along the simulation texture
			deviceContext->CSSetUnorderedAccessViews(1, 1, turingTexUAV.GetAddressOf(), 0);
			deviceContext->CSSetConstantBuffers(0, 1, simInput.GetAddressOf());
			// Iterate the Gray-Scott reaction-diffusion system
			// (but only if pattern simulation is un-paused)
			deviceContext->Dispatch(ITER_GROUP_WIDTH,
									ITER_GROUP_WIDTH,
									1);
			deviceContext->Dispatch(ITER_GROUP_WIDTH,
									ITER_GROUP_WIDTH,
									1);
			deviceContext->Dispatch(ITER_GROUP_WIDTH,
									ITER_GROUP_WIDTH,
									1);
			deviceContext->Dispatch(ITER_GROUP_WIDTH,
									ITER_GROUP_WIDTH,
									1);
			// Resources can't be exposed through unordered-access-views (read/write allowed) and shader-
			// resource-views (read-only) simultaneously, so un-bind the the Turing texture's
			// unordered-access-view over here
			Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> nullUAV = nullptr;
			deviceContext->CSSetUnorderedAccessViews(1, 1, &nullUAV, 0);
		}
	}
}

void DXPanel::SynthFurDXPanel::MeshPresent()
{
	// Increment camera orbit, generate orbital matrix
	// (if the surface is spinning)
	// Accesses cached delta-time from reaction/diffusion iteration
	camOrbi += (surfSpinning) ? simSettings->timeMaxCoord.x : 0.0f;
	DirectX::XMMATRIX orbiMat = DirectX::XMMatrixRotationY(camOrbi * -1.0f);
	// Reconstruct view-matrix with current zoom depth
	viewMat = DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.0f, 0.0f, camZoom, 0.0f),
										DirectX::XMVectorSet(0.0f, 0.01f, 0.0f, 0.0f),
										DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	// Apply orbit
	viewMat = DirectX::XMMatrixMultiply(orbiMat, viewMat);
	// Transpose; differing matrix order on CPU/GPU
	viewMat = DirectX::XMMatrixTranspose(viewMat);
	if (!gpuPaused)
	{
		// Update GPU-side view/perspective matrices
		D3D11_MAPPED_SUBRESOURCE input;
		HRESULT result = deviceContext->Map(projMatInput.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &input);
		assert(SUCCEEDED(result));
		MeshTransf* projBuf = (MeshTransf*)input.pData;
		projBuf->view = viewMat;
		projBuf->persp = perspProj;
		deviceContext->Unmap(projMatInput.Get(), 0);
		// Prepare read-only view over the Turing texture
		deviceContext->PSSetShaderResources(0, 1, turingTexSRV.GetAddressOf());
		// Pass vertex + index buffers to the GPU
		unsigned int stride = sizeof(WaveFrontReader<unsigned int>::Vertex);
		unsigned int offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, mesh->VtBuffer().GetAddressOf(), &stride, &offset);
		deviceContext->IASetIndexBuffer(mesh->NdxBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		// Set primitive topology
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// Prepare shader for the GPU + rasterize
		meshPresentation->Raster(deviceContext,
								 projMatInput,
								 simInput,
								 turingTexSRV,
								 oTexUAV,
								 mesh->NumIndices());
	}
}

void DXPanel::SynthFurDXPanel::SubmitGPUWork()
{
	// Layout empty parameters for submission
	DXGI_PRESENT_PARAMETERS params = { 0 };
	params.DirtyRectsCount = 0;
	params.pDirtyRects = nullptr;
	params.pScrollRect = nullptr;
	params.pScrollOffset = nullptr;

	// Submit buffered dispatch/draw data to the GPU with unit-interval vsync + zeroed
	// submission parameters
	HRESULT result = swapChain->Present1(1, 0, &params);

	// Flipped-sequential multi-buffering unbinds the render-target, so rebind it here
	deviceContext->OMSetRenderTargets(1, defaultRenderTarget.GetAddressOf(), depthStencil.Get());

	// We've rendered, so clear the render-target-view before the next scene
	// Light gray background for better contrast with characters
	float color[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	deviceContext->ClearRenderTargetView(defaultRenderTarget.Get(), color);

	// Special case failures for device removal/device reset
	if (result == DXGI_ERROR_DEVICE_REMOVED ||
		result == DXGI_ERROR_DEVICE_RESET)
	{
		ClearAndReInitDX();
	}
	else
	{
		assert(SUCCEEDED(result));
	}

	// We've pushed through all the work we need to do for now, so increment the frame counter
	// here
	simSettings->turingCenter.y += 1;
}

void DXPanel::SynthFurDXPanel::InitTexSim()
{
	// Construct + associate swap-chain, generate related textures
	InitDX();

	// Load the user's mesh
	mesh = new Mesh(meshSrc->Data(), device);

	// Prepare simulation settings
	simSettings = new SimSettings();

	// Initialize palettes to white
	float u = 1.0f;
	DirectX::XMVECTOR vU = _mm_broadcast_ss(&u);
	DirectX::XMStoreFloat4(&(simSettings->palette0), vU);
	DirectX::XMStoreFloat4(&(simSettings->palette1), vU);
	DirectX::XMStoreFloat4(&(simSettings->palette2), vU);

	// Cache minimum/maximum bounding coordinates
	DirectX::BoundingBox bb = mesh->BBox();
	DirectX::XMFLOAT3 bbCrnrs[8];
	bb.GetCorners(bbCrnrs);
	DirectX::XMFLOAT3 minCrnr = bbCrnrs[4];
	DirectX::XMFLOAT3 maxCrnr = bbCrnrs[2];
	simSettings->timeMaxCoord.y = maxCrnr.x;
	simSettings->timeMaxCoord.z = maxCrnr.y;
	simSettings->timeMaxCoord.w = maxCrnr.z;
	simSettings->minCoordLiTheta.x = minCrnr.x;
	simSettings->minCoordLiTheta.y = minCrnr.y;
	simSettings->minCoordLiTheta.z = minCrnr.z;

	// Initialize the frame-counter to zero
	simSettings->turingCenter.y = 0;

	// Load the texture generator
	texGen = new ComputeShader(device, L"TuringTexSim.cso");

	// Load the presentation shader
	meshPresentation = new RasterShader(device, L"MeshPresentVts.cso", L"MeshPresentPxls.cso");

	// Prepare settings input
	D3D11_BUFFER_DESC inputBufferDesc;
	inputBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	inputBufferDesc.ByteWidth = sizeof(SimSettings);
	inputBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	inputBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	inputBufferDesc.MiscFlags = 0;
	inputBufferDesc.StructureByteStride = 0;
	HRESULT result = device->CreateBuffer(&inputBufferDesc, nullptr, simInput.GetAddressOf());
	assert(SUCCEEDED(result));

	// Prepare projection input
	inputBufferDesc.ByteWidth = sizeof(MeshTransf); // Only byte-width changes between each buffer
	result = device->CreateBuffer(&inputBufferDesc, nullptr, projMatInput.GetAddressOf());
	assert(SUCCEEDED(result));

	// Prepare simulation texture
	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = TURING_TEX_WIDTH;
	textureDesc.Height = TURING_TEX_HEIGHT;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0; // Maximum quality level for the chosen MSAA sampling rate
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	result = device->CreateTexture2D(&textureDesc, nullptr, turingTex.GetAddressOf());
	assert(SUCCEEDED(result));
	result = device->CreateShaderResourceView(turingTex.Get(), nullptr, turingTexSRV.GetAddressOf());
	assert(SUCCEEDED(result));
	result = device->CreateUnorderedAccessView(turingTex.Get(), nullptr, turingTexUAV.GetAddressOf());
	assert(SUCCEEDED(result));

	// Prepare output texture (only changing properties are re-set)
	textureDesc.Width = OUTPUT_TEX_WIDTH;
	textureDesc.Height = OUTPUT_TEX_HEIGHT;
	textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	result = device->CreateTexture2D(&textureDesc, nullptr, oTex.GetAddressOf());
	assert(SUCCEEDED(result));
	// No shader-resource-view for the output texture
	result = device->CreateUnorderedAccessView(oTex.Get(), nullptr, oTexUAV.GetAddressOf());
	assert(SUCCEEDED(result));

	// Prepare the GPU worker thread
	auto asyncHandler = ref new WorkItemHandler([this](IAsyncAction^ action)
	{
		while (action->Status == AsyncStatus::Started)
		{
			if (simSettings->turingCenter.y == 0) { InitSettings(); } // Initialize settings on the zeroth frame
			bool reset = simSettings->turingCenter.z == 1; // Cache whether or not pattern reset is active for the current frame
			IterTexSim(); // Iterate the texture sim
			MeshPresent(); // Present the user's mesh with the synthesized texture
			if (!gpuPaused) { SubmitGPUWork(); } // Submit prepared work to the GPU
			if (reset) { simSettings->turingCenter.z = 0; } // Revert pattern reset if it was active for the most recent frame
		}
	});
	simLoopWorker = ThreadPool::RunAsync(asyncHandler, WorkItemPriority::High); // High priority because low-latency texture synthesis
	// Update initialization status
	initted = true;
}

bool DXPanel::SynthFurDXPanel::CheckInit()
{
	return initted;
}

void DXPanel::SynthFurDXPanel::CacheMesh(Platform::String^ userMeshSrc)
{
	meshSrc = userMeshSrc;
}

void DXPanel::SynthFurDXPanel::SwitchTexSim()
{
	gpuPaused = !gpuPaused;
}

void DXPanel::SynthFurDXPanel::SwitchPatternSim()
{
	patternSimPaused = !patternSimPaused;
}

Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface^ DXPanel::SynthFurDXPanel::ExportTex(int id)
{
	EXPORTABLE_TEX_IDS expTexID = (EXPORTABLE_TEX_IDS)id;
	const Microsoft::WRL::ComPtr<ID3D11Texture2D>& tex = (expTexID == EXPORTABLE_TEX_IDS::OUTPUT) ? oTex : turingTex; // Will likely nest these later for more
																													  // output options while keeping [tex] constant &
																													  // initialized on creation
	Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurf;
	HRESULT result = tex->QueryInterface<IDXGISurface>(dxgiSurf.GetAddressOf());
	assert(SUCCEEDED(result));
	return Windows::Graphics::DirectX::Direct3D11::CreateDirect3DSurface(dxgiSurf.Get());
}

void DXPanel::SynthFurDXPanel::SetFurPalette0(float r, float g, float b)
{
	DirectX::XMFLOAT4& palette = simSettings->palette0;
	palette.x = r / 255.0f;
	palette.y = g / 255.0f;
	palette.z = b / 255.0f;
}

void DXPanel::SynthFurDXPanel::SetFurPalette1(float r, float g, float b)
{
	DirectX::XMFLOAT4& palette = simSettings->palette1;
	palette.x = r / 255.0f;
	palette.y = g / 255.0f;
	palette.z = b / 255.0f;
}

void DXPanel::SynthFurDXPanel::SetTuringPalette(float r, float g, float b)
{
	DirectX::XMFLOAT4& palette = simSettings->palette2;
	palette.x = r / 255.0f;
	palette.y = g / 255.0f;
	palette.z = b / 255.0f;
}

void DXPanel::SynthFurDXPanel::SetTuringReactivity(float r)
{
	simSettings->palette2.w = r;
}

void DXPanel::SynthFurDXPanel::SetTuringDiffusionU(float ru)
{
	simSettings->turingParams.x = ru;
}

void DXPanel::SynthFurDXPanel::SetTuringDiffusionV(float rv)
{
	simSettings->turingParams.y = rv;
}

void DXPanel::SynthFurDXPanel::SetTuringConvRate(float c)
{
	simSettings->turingParams.z = c;
}

void DXPanel::SynthFurDXPanel::SetTuringFeedRate(float f)
{
	simSettings->turingParams.w = f;
}

void DXPanel::SynthFurDXPanel::SetTuringBlendOp(unsigned int op)
{
	simSettings->fadingTuring.z = (simSettings->fadingTuring.z > MAX_BLEND_ID) ? op + (float)NUM_BLENDS : (float)op;
}

void DXPanel::SynthFurDXPanel::SetBaseShadingAxis(unsigned int axis)
{
	simSettings->fadingBase.y = (float)axis;
}

void DXPanel::SynthFurDXPanel::SetBaseFadeState(unsigned int state)
{
	simSettings->palette1.w = (float)state;
}

void DXPanel::SynthFurDXPanel::SetBaseFadeRate(float m)
{
	simSettings->fadingBase.x = m;
}

void DXPanel::SynthFurDXPanel::SwitchBaseShadingDirection()
{
	simSettings->fadingBase.z = !simSettings->fadingBase.z;
}

void DXPanel::SynthFurDXPanel::SwitchTuringShadingDirection()
{
	simSettings->fadingTuring.w = !simSettings->fadingTuring.w;
}

void DXPanel::SynthFurDXPanel::SetTuringShadingAxis(unsigned int axis)
{
	simSettings->fadingTuring.y = (float)axis;
}

void DXPanel::SynthFurDXPanel::SetTuringFadeRate(float m)
{
	simSettings->fadingTuring.x = m;
}

void DXPanel::SynthFurDXPanel::SwitchTuringPatterns()
{
	// Set turing blend operation out-of-range (indicating no blending) if appropriate,
	// otherwise return it to the value set before blending operations were averted
	simSettings->fadingTuring.z += (simSettings->fadingTuring.z > MAX_BLEND_ID) ? -((float)NUM_BLENDS) : (float)NUM_BLENDS;
}

void DXPanel::SynthFurDXPanel::SwitchSurfaceClamping()
{
	simSettings->fadingBase.w = !simSettings->fadingBase.w;
}

void DXPanel::SynthFurDXPanel::SetLiIntensity(float f)
{
	simSettings->palette0.w = f;
}

void DXPanel::SynthFurDXPanel::UpdateViewZoom(signed int dZoom)
{
	camZoom += ((float)dZoom) / 5000.0f; // Delta-time may or may not have been initialized at zoom time; smooth regular zoom
										 // is better than jerky zoom that keeps time with framerate
}

void DXPanel::SynthFurDXPanel::SetTuringKrnCenter(int i)
{
	simSettings->turingCenter.x = i;
}

void DXPanel::SynthFurDXPanel::SetTuringKrnElt(int i, unsigned int eltID)
{
	switch (eltID)
	{
		case 0:
			simSettings->turingKrn.x = i;
			break;
		case 1:
			simSettings->turingKrn.y = i;
			break;
		case 2:
			simSettings->turingKrn.z = i;
			break;
		case 3:
			simSettings->turingKrn.w = i;
			break;
		default:
			assert(false); // Element ID out-of-range
	}
}

void DXPanel::SynthFurDXPanel::SetTuringKrn2Elt(int i, unsigned int eltID)
{
	switch (eltID)
	{
		case 0:
			simSettings->turingKrn2.x = i;
			break;
		case 1:
			simSettings->turingKrn2.y = i;
			break;
		case 2:
			simSettings->turingKrn2.z = i;
			break;
		case 3:
			simSettings->turingKrn2.w = i;
			break;
		default:
			assert(false); // Element ID out-of-range
	}
}

void DXPanel::SynthFurDXPanel::TuringReset()
{
	simSettings->turingCenter.z = 1; // Un-setting this happens automatically within the rendering worker thread
									 // (because Turing patterns are integrated in time, so they can't regenerate over
									 // the surface until they have a few frames to develop)
}

void DXPanel::SynthFurDXPanel::SetTuringProjection(unsigned int proj)
{
	simSettings->turingCenter.w = proj;
}

void DXPanel::SynthFurDXPanel::SwitchSurfSpin()
{
	surfSpinning = !surfSpinning;
}

void DXPanel::SynthFurDXPanel::ClearAndReInitDX()
{
	// Nullify the swap-chain
	swapChain = nullptr;

	// Empty rendering state
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	deviceContext->Flush();

	// Re-initialize swap-chain
	InitDX();
}

void DXPanel::SynthFurDXPanel::SizeUpdate()
{
	// Resize the swap-chain
	HRESULT result = swapChain->ResizeBuffers(2,
											  (UINT)ActualWidth,
											  (UINT)ActualHeight,
											  SWAP_CHAIN_FORMAT,
											  0);
	// Special case failures for device removal/device reset
	if (result == DXGI_ERROR_DEVICE_REMOVED ||
		result == DXGI_ERROR_DEVICE_RESET)
	{
		// Clear gpu state and re-initialize
		ClearAndReInitDX();
	}
	else
	{
		assert(SUCCEEDED(result));
	}
}

void DXPanel::SynthFurDXPanel::OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	if (initted) { return; }
	else { InitTexSim(); }
}

void DXPanel::SynthFurDXPanel::OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Platform::Object^ args) {}

void DXPanel::SynthFurDXPanel::OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e)
{
	// Minimize memory usage during suspension
	if (initted)
	{
		Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
		device.As(&dxgiDevice);
		dxgiDevice->Trim();
	}
}

void DXPanel::SynthFurDXPanel::OnResuming(Platform::Object^ sender, Platform::Object^ args) {}
