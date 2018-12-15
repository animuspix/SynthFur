
cbuffer SimSettings
{
    float4 timeMaxCoord; // Delta-time in [x], maximum bounding coordinate in [yzw]
    float4 minCoordLiTheta; // Minimum bounding coordinate in [xyz], light rotation angle in [w]
    float4 palette0; // First palette color, used to shade animal upper-side ([w] carries view lighting intensity)
    float4 palette1; // Second palette color, used to shade animal under-side ([w] describes whether to use both palette
                     // shades (0), just the first (1), or just the second(2))
    float4 palette2; // Third palette color, used for Turing spots/stripes (background Turing color is white/paletted);
                     // [w] describes white-noise reactivity variance (hashed for now, possibly PRNG-driven in future)
    float4 turingParams; // Turing function parameters; from left-to-right:
    		             //- Diffusion rate for the zeroth Turing chemical
    		             //- Diffusion rate for the first Turing chemical
    		             //- Reaction/conversion frequency (noise-modulated) between secondary chemicals
                         //- Supply rate for the background turing chemical (baseline animal shading)
    // Surprising and unpredictable errors with floating-point kernel weights; encoding as integer ratios instead
	// Kernel denominator is implicitly [1000]
	int4 turingCenter; // Central Turing weight in [x], [y] carries the frame counter, [z] carries whether to
                       // to reset Turing pattern generation, [w] carries the user's pattern/surface projection function
                       // (arbitrary/UVs, triplanar mapping, planar mapping)
	int4 turingKrn; // Nearby Turing kernel parameters; from left-to-right:
					//- Leftward weight
					//- Rightward weight
					//- Upward weight
					//- Downward weight
	int4 turingKrn2; // Corner paramters for the Turing kernel
					 //- Upper-left corner
					 //- Upper-right corner
					 //- Lower-left corner
					 //- Lower-right corner
    float4 fadingBase; // Upside/under-side fade settings for base shading; gradient strength in [x], shading axis index in [y], shading axis
                       // polarity in [z], whether or not to clamp SynthFur colors in [w]
    float4 fadingTuring; // Upside/under-side fade settings for Turing patterns; gradient strength in [x], shading axis index in [y], blend type for Turing
                         // patterns (multiply/divide/add/subtract/[no blending]) in [z], shading axis polarity in [w]
};