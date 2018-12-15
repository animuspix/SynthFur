
#include "SharedShading.hlsli"

struct Pixel
{
    float4 posProj : SV_POSITION0;
    float3 posBase : POSITION1;
    float3 normal : NORMAL0;
    float2 txCoord : TEXCOORD0;
};

// Read-only view over [turingTex]
Texture2D<float2> turingTex : register(t0);

// State definition for the texture sampler used with [turingTex] read-only accesses
SamplerState wrapSampler : register(s0);

// Output texture, contains composite normal/Turing-shaded surface projected through
// the UVs associated with the user's mesh
RWTexture2D<float4> oTex : register(u1);

// Pattern->surface projection function enum
#define PROJ_MESH_UVS 0
#define PROJ_TRIPLANAR 1
#define PROJ_MONOPLANAR 2

// Palette activity enum
#define BOTH_PALETTE_ELTS 0
#define ONLY_PALETTE_ZERO 1
#define ONLY_PALETTE_ONEE 2

// Blending operation enum
#define BLEND_MUL 0
#define BLEND_DIV 1
#define BLEND_ADD 2
#define BLEND_SUB 3

float4 main(Pixel p) : SV_TARGET
{
    // Update input positions/bounds for shading
    float3 scale = abs(minCoordLiTheta.xyz) * 2.0f; // Compute mesh scale from bounds
    float3 minCoord = minCoordLiTheta.xyz / scale; // Scale bounds into [-0.5...0.5]
    float3 maxCoord = timeMaxCoord.yzw / scale;
    float3 relCoord = (p.posBase.xyz / scale) + maxCoord; // Scale figure position + offset to be zero-relative
    minCoord += maxCoord; // Offset bounds to be zero-relative
    maxCoord += maxCoord;

    // Construct Turing pattern sample using the user's projection function
    float2 gs = 1.0f.xx;
    switch (turingCenter.w)
    {
        case PROJ_MESH_UVS: // Projection through surface UV coordinates
            gs = turingTex.Sample(wrapSampler, p.txCoord);
            break;
        case PROJ_TRIPLANAR: // Projection with triplanar mapping
            float3 unitPosBase = normalize(p.posBase.xyz);
            float3x3 turingKrn = float3x3(float3(turingTex.Sample(wrapSampler, abs(unitPosBase.yz)), 0.0f),
                                          float3(turingTex.Sample(wrapSampler, abs(unitPosBase.xz)), 0.0f),
                                          float3(turingTex.Sample(wrapSampler, abs(unitPosBase.xy)), 0.0f));
            gs = min(mul(abs(p.normal.xyz), turingKrn), 1.0f.xxx).xy;
            break;
        case PROJ_MONOPLANAR: // Projection with naive planar mapping across XY
            gs = turingTex.Sample(wrapSampler, relCoord.xy);
            break;
        default: // Use surface UV values by default
            gs = turingTex.Sample(wrapSampler, p.txCoord);
            break;
    }
    // Apply the given pattern color over the sampled UV value
    float3 gsRGB = gs.y * palette2.rgb;

    // Construct basic normal-shaded color
    // Assumes central mesh anchors
    float3 dirRGB = 0.0f.xxx;
    switch (palette1.w)
    {
        case BOTH_PALETTE_ELTS: // Fade palettes together along the given axis
            float baseFadeWeight = min(relCoord[(int)fadingBase.y] * fadingBase.x, 1.0f); // Position axes are all in 0...1 and fit within surface boundaries,
                                                                                          // no need for division here
                                                                                          // Saturated gradient scaling for shortened interpolation
            dirRGB = lerp(palette0.rgb, palette1.rgb, fadingBase.z ? (1.0f - baseFadeWeight) : baseFadeWeight);
            break;
        case ONLY_PALETTE_ZERO: // Assign directly from palette color #0
            dirRGB = palette0.rgb;
            break;
        case ONLY_PALETTE_ONEE: // Assign directly from palette color #1
            dirRGB = palette1.rgb;
            break;
        default: // Assign from palette color #0 by default
            dirRGB = palette0.rgb;
            break;
    }

    // Cache fading weight for Turing shading
    // Lock fading weight to one (no visible pattern) whenever the [u] component of the turing pattern is near one
    // and the [v] component is near zero (indicates background area)
    float gsDiff = (gs.x - gs.y);
    float turingFadeWeight = relCoord[(int)fadingTuring.y];
    turingFadeWeight = fadingTuring.w ? (1.0f - turingFadeWeight) : turingFadeWeight;
    turingFadeWeight = saturate((turingFadeWeight * fadingTuring.x) + gsDiff); // Saturating gradient scaling for shortened interpolation

    // Possibility for multiple pattern support + blending here

    // Blend directional + Turing colors appropriately; apply Turing fade while blending
    float3 rgb = dirRGB;
    switch (fadingTuring.z) // Switch on operation type
    {
        case BLEND_MUL: // Multiplicative blend
            rgb *= lerp(gsRGB, 1.0f.xxx, turingFadeWeight);
            break;
        case BLEND_DIV: // Divisional blend, extended to avoid divisions by zero
            gsRGB = lerp(gsRGB, 1.0f.xxx, turingFadeWeight);
            gsRGB.x = (gsRGB.x == 0.0f) ? 1.0f : gsRGB.x;
            gsRGB.y = (gsRGB.y == 0.0f) ? 1.0f : gsRGB.y;
            gsRGB.z = (gsRGB.z == 0.0f) ? 1.0f : gsRGB.z;
            rgb /= gsRGB;
            break;
        case BLEND_ADD: // Additive blend
            rgb += lerp(gsRGB, 0.0f.xxx, turingFadeWeight);
            break;
        case BLEND_SUB: // Subtractive blend
            rgb -= lerp(gsRGB, 0.0f.xxx, turingFadeWeight);
            break;
        default: // We manually set default values for each setting (including blending operations), so
                 // treat any values besides mul/div/add/sub as ignoring blending
            rgb = rgb;
            break;
    }

    // Optionally constrain colors to the range [0...1]
    rgb = (fadingBase.w) ? min(abs(rgb), 1.0f.xxx) : rgb;

    // Record synthesized color in [oTex]
    const uint oTexWidth = 8192;
    oTex[p.txCoord * oTexWidth] = rgb.rgbb;

    // Output local surface shading with basic directional illumination
    float3 liDir = float3(sin(minCoordLiTheta.w),
                          0.0f,
                          cos(minCoordLiTheta.w));
    rgb = rgb * dot(p.normal.xyz * -1.0f, liDir) * palette0.w;
    rgb = (fadingBase.w) ? saturate(rgb) : rgb; // Optionally constrain lighting to the range [0...1]
    return rgb.rgbb;
}