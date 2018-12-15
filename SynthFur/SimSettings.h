
#pragma once

#include <DirectXMath.h>
#include "pch.h"

namespace DXPanel
{
	// Packaging type for user-accessible settings
	struct SimSettings
	{
		DirectX::XMFLOAT4 timeMaxCoord; // Delta-time in [x], maximum bounding coordinate in [yzw]
    	DirectX::XMFLOAT4 minCoordLiTheta; // Minimum bounding coordinate in [xyz], light rotation angle in [w]
		DirectX::XMFLOAT4 palette0; // First palette color, used to shade animal upper-side ([w] carries view lighting intensity)
		DirectX::XMFLOAT4 palette1; // Second palette color, used to shade animal under-side ([w] describes whether to use both palette
									// shades (0), just the first (1), or just the second(2))
		DirectX::XMFLOAT4 palette2; // Third palette color, used for Turing spots/stripes (background Turing color is white/paletted);
									// [w] describes white-noise reactivity variance (hashed for now, possibly PRNG-driven in future)
		DirectX::XMFLOAT4 turingParams; // Turing function parameters; from left-to-right:
										//- Diffusion rate for the zeroth Turing chemical
										//- Diffusion rate for the first Turing chemical
										//- Reaction/conversion frequency (noise-modulated) between secondary chemicals
										//- Supply rate for the background turing chemical (baseline animal shading)
		// Surprising and unpredictable errors with floating-point kernel weights; encoding as integer ratios instead
		// Kernel denominator is implicitly [1000]
		DirectX::XMINT4 turingCenter; // Central Turing weight in [x], [y] carries the frame counter, [z] carries whether to
									  // to reset Turing pattern generation, [w] carries the user's pattern/surface projection function
									  // (arbitrary/UVs, triplanar mapping, planar mapping)
		DirectX::XMINT4 turingKrn; // Nearby Turing kernel parameters; from left-to-right:
								   //- Leftward weight
								   //- Rightward weight
								   //- Upward weight
								   //- Downward weight
		DirectX::XMINT4 turingKrn2; // Corner paramters for the Turing kernel
									//- Upper-right corner
									//- Lower-left corner
									//- Lower-right corner
									//- Upper-left corner
		DirectX::XMFLOAT4 fadingBase; // Upside/under-side fade settings for base shading; gradient strength in [x], shading axis index in [y], shading axis
									  // polarity in [z], whether or not to clamp SynthFur colors in [w]
		DirectX::XMFLOAT4 fadingTuring; // Upside/under-side fade settings for Turing patterns; gradient strength in [x], shading axis index in [y], blend type for Turing
										// patterns (multiply/divide/add/subtract/[no blending]) in [z], shading axis polarity in [w]
	};

}
