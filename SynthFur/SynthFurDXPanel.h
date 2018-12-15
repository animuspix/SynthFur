#pragma once

#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3dcommon.h>
#include <d3d11_2.h>
#include <directxmath.h>
#include <chrono>
#include <wrl/client.h>
#include "pch.h"
#include <windows.ui.xaml.media.dxinterop.h>
#include <windows.graphics.directx.direct3d11.interop.h>

namespace DXPanel
{
	enum class EXPORTABLE_TEX_IDS // Exportable texture enumeration, used to choose between the main
								  // output texture and the Turing-pattern texture on export
	{
		TURING,
		OUTPUT
	};

	struct SimSettings;
	class Mesh;
	struct ComputeShader;
	class RasterShader;
	public ref class SynthFurDXPanel sealed : public Windows::UI::Xaml::Controls::SwapChainPanel
	{
		public:
			SynthFurDXPanel(); // Minimal constructor, just prepares panel event handlers
			// No public destructors for managed C++/CX objects
			void InitTuringSettings(); // Specialty initializer for Turing-specific settings, allows the user to reset pattern
									   // parameters instantly instead of trying to find each original setting by hand
			// Check initialization status
			bool CheckInit();
			// Cache user mesh
			void CacheMesh(Platform::String^ userMeshSrc);
			// Pause all GPU work
			void SwitchTexSim();
			// Pause pattern simulation
			void SwitchPatternSim();
			// Export output/turing textures
			Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface^ ExportTex(int id);
			// Update simulation settings
			void SetFurPalette0(float r, float g, float b); // Palette settings
			void SetFurPalette1(float r, float g, float b);
			void SetTuringPalette(float r, float g, float b);
			void SetTuringReactivity(float r); // RDE-specific settings
			void SetTuringDiffusionU(float ru);
			void SetTuringDiffusionV(float rv);
			void SetTuringConvRate(float c);
			void SetTuringFeedRate(float f);
			void SetTuringBlendOp(unsigned int op);
			void SetBaseShadingAxis(unsigned int axis); // Fade axis for main fur palette
			void SetBaseFadeState(unsigned int state); // Fade state for main fur palette (fading, only palette0, only palette1)
			void SetBaseFadeRate(float m); // Fading gradient for main fur
			void SwitchBaseShadingDirection(); // Shading direction associated with the given axis for main fur (e.g. forward or backward along [z])
			void SwitchTuringShadingDirection(); // Shading direction associated with the given axis for patterned fur (e.g. forward or backward along [z])
			void SetTuringShadingAxis(unsigned int axis); // Fade axis for Turing patterns/basis fur
			void SetTuringFadeRate(float m); // Fading gradient for Turing patterns
			void SwitchTuringPatterns(); // Shading direction associated with the given axis for Turing patterns
			void SwitchSurfaceClamping(); // Turn clamping on/off for synthetic colors
			void SetLiIntensity(float f); // Lighting intensity used in the model/texture view
			void UpdateViewZoom(signed int dZoom); // Update camera zoom into the scene
			void SetTuringKrnCenter(int i); // Assign [f] as the central weight in the pattern kernel
			void SetTuringKrnElt(int i, unsigned int eltID); // Assign [f] into the specified adjacent element
			void SetTuringKrn2Elt(int i, unsigned int eltID); // Assign [f] into the specified diagonal element
			void TuringReset(); // Reset Turing pattern synthesis in the next frame
			void SetTuringProjection(unsigned int proj); // Set the function to use when projecting 2D turing patterns to 3D
			void SwitchSurfSpin(); // Switch surface rotation on/off
		private protected:
			// Initialize swap-chain + texture simulation
			// Private because initialization can't occur until after the first
			// resize, which makes it natural to privately + conditionally resize
			// within OnSizeChanged(...) instead of within external page setup
			void InitTexSim();
			// Initialize settings
			// Settings are initialized on the zeroth frame within the rendering worker thread, so no need to make this public
			void InitSettings();
			// Core initializer, replaces constructor (called after the user selects a mesh)
			void InitDX();
			// Step through texture simulation
			void IterTexSim();
			// Draw textured mesh to the swap-chain panel
			void MeshPresent();
			// Commit draw/dispatch calls to the GPU
			void SubmitGPUWork();
			// Buffer resizer, useful when page layout changes or composition scale updates
			void SizeUpdate();
			// DirectX resetter/re-initializer, useful whenever SynthFur loses GPU access
			void ClearAndReInitDX();
			// Direct3D internal types
			Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
			Microsoft::WRL::ComPtr<ID3D11Device> device;
			Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> defaultRenderTarget;
			Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencil;
			// UWP event handlers
			void OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
			void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Platform::Object^ args);
			void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
			void OnResuming(Platform::Object^ sender, Platform::Object^ args);
			// Simulation-loop worker thread
			Windows::Foundation::IAsyncAction^ simLoopWorker;
			// Flag giving whether or not [this] is fully initialized
			bool initted;
			// Flag giving whether or not pattern simulation is paused
			bool patternSimPaused;
			// Flag giving whether or not all GPU activity is paused (useful for minimizing GPU load when other programs are running)
			bool gpuPaused;
			// SynthFur-specific data
			SimSettings* simSettings; // Simulation properties
			Mesh* mesh; // The user's mesh
			ComputeShader* texGen; // Compute shader to generate/adjust the mesh's texture
			RasterShader* meshPresentation; // Rasterization shader to present/texturize the user's mesh
			Microsoft::WRL::ComPtr<ID3D11Buffer> simInput; // Simulation input values
			Microsoft::WRL::ComPtr<ID3D11Buffer> projMatInput; // Constant-buffer carrying projection matrix for presentation
			Microsoft::WRL::ComPtr<ID3D11Texture2D> turingTex; // Turing pattern solution space
			Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> turingTexUAV; // Shader-friendly UAV over [turingTex]
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> turingTexSRV; // Read-only rasterization view over [turingTex]
			Microsoft::WRL::ComPtr<ID3D11Texture2D> oTex; // Output space for synthesized textures
			Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> oTexUAV; // Shader-friendly UAV over the output texture
			DirectX::XMMATRIX perspProj; // Perspective projection matrix
			DirectX::XMVECTOR camPos; // Camera position
			DirectX::XMMATRIX viewMat; // View projection matrix (for camera movement)
			Platform::String^ meshSrc; // User mesh source (neeeded for simulation setup)
			float camZoom; // Camera zoom along the viewing direction ([z] by default)
			float camOrbi; // Orbital camera angle around the user's mesh
			bool surfSpinning; // Whether or not user's surface is spinning within the view
		private:
			struct MeshTransf // Small local type to allow clean access for cbuffer'd projection data
			{
				DirectX::XMMATRIX view;
				DirectX::XMMATRIX persp;
			};
			// Small helper method for concise color assignments
			void rgbAssign(DirectX::XMFLOAT4& dst,
						   DirectX::XMVECTOR rgb);
			// Maximum non-null blending ID
			static constexpr unsigned int MAX_BLEND_ID = 3;
			// Number of blend types supported by SynthFur (excluding null blending)
			static constexpr unsigned int NUM_BLENDS = 4;
			// Swap chain format given standard UWP color order (BGRA/ARGB)
			static constexpr DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
			// Turing texture width/height (4096x4096) (above 4K, mastering resolution)
			static constexpr unsigned int TURING_TEX_WIDTH = 4096;
			static constexpr unsigned int TURING_TEX_HEIGHT = 4096;
			// Turing iteration group width
			// 256 threads (8*8*4) are deployed for each group, so ideal group deployment would be
			// (64, 64) (4096)
			static constexpr unsigned int ITER_GROUP_WIDTH = 256;
			// Output texture width/height (8192x8192) (much higher-resolution than 4K, able to capture turing patterns
			// + extra detail from directional shading)
			static constexpr unsigned int OUTPUT_TEX_WIDTH = 8192;
			static constexpr unsigned int OUTPUT_TEX_HEIGHT = 8192;
			// Clock + time capture, used for evaluating delta-time
			std::chrono::steady_clock simClock;
			std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
			// Private destructor
			~SynthFurDXPanel();
	};

	// A small global constant giving the base used for rational values in SynthFur
	// (like kernel weights)
	static constexpr int RATIONAL_DENOM = 1000;
}
