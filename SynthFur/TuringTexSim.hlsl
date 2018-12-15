
#include "SharedShading.hlsli"

// Shading texture, used for Turing function integration
RWTexture2D<float2> turingTex : register(u1);

// Texture size is hardcoded 4096x4096
// Group size is 8x8x4 = 256 threads (expecting too much work for 512)
// 16x16 groups expected (256)
// Thread deployment is (256, 256, 0), because it's easy and we're just doing
// short operations in 2D
#define DOMAIN_AREA 0x1000.xx // 4096 in hexadecimal
#define GROUP_WIDTH_XY 16
#define MAX_TXL_ID 16777215

[numthreads(8, 8, 4)]
void main(uint3 groupID : SV_GroupID,
          uint threadID : SV_GroupIndex)
{
    // Fold global thread ID into a 2D texel index, mask excess
    int2 texNdx = int2((groupID.x * GROUP_WIDTH_XY) + threadID % GROUP_WIDTH_XY,
                       (groupID.y * GROUP_WIDTH_XY) + threadID / GROUP_WIDTH_XY);
    int linTexNdx = texNdx.x + texNdx.y * DOMAIN_AREA.x;
    if (linTexNdx > MAX_TXL_ID) { return; }
    // Perform Gray-Scott reaction-diffusion
    // Gray-scott differentiated form found in:
    // https://groups.csail.mit.edu/mac/projects/amorphous/GrayScott/
    // u' = ru(Lap(u)) - uv^2 + f(1 - u)
    // v' = rv(Lap(v)) - uv^2 - (f + k)v
    // Seed the texture on the zeroth frame, then exit early
    // (also perform seeding if the user asks to reset)
    // Seeding uses the filled rectangle intersector found here:
    // https://www.shadertoy.com/view/MssyDf
    if (turingCenter.y == 0 || turingCenter.z)
    {
        if (texNdx.x > 1843 && texNdx.x < 2254 &&
            texNdx.y > 1843 && texNdx.y < 2254) {
            turingTex[texNdx] = 1.0f.xx;
            return;
        }
        else
        {
            turingTex[texNdx] = 0.0f.xx;
            return;
        }
    }
    // Cache local Gray-Scott value
    float4 testTuringKrn = (float4)turingKrn / 1000.0f;
    float4 testTuringKrn2 = (float4)turingKrn2 / 1000.0f;
    float2 gs = turingTex[texNdx];
    // Compute skewed laplacian
    float2 lap = turingTex[texNdx + int2(0, 1)] * testTuringKrn.x + // Vertical neighbours
                 turingTex[texNdx + int2(0, -1)] * testTuringKrn.y +
                 turingTex[texNdx + int2(1, 0)] * testTuringKrn.z + // Horizontal neighbours
                 turingTex[texNdx + int2(-1, 0)] * testTuringKrn.w +
                 turingTex[texNdx + int2(1, 1)] * testTuringKrn2.x + // Diagonal neigbours
                 turingTex[texNdx + int2(-1,-1)] * testTuringKrn2.y +
                 turingTex[texNdx + int2(1, -1)] * testTuringKrn2.z +
                 turingTex[texNdx + int2(-1, 1)] * testTuringKrn2.w;
    lap += gs * turingCenter.x; // Difference between local cells and the current cell,
                                // after re-weighting the current cell
    float xyy = gs.x * gs.y * gs.y; // Compute dynamic conversion probability between [u] and [v]
                                    // Applied as an offset towards the conversion function
    float feed = turingParams.w * (1.0f - gs.x); // Compute feed value for [u]
    float conv = (turingParams.z + turingParams.w) * gs.y; // Compute conversion toward [v]
    float2 delta = float2(turingParams.x * lap.x - xyy + feed,
                          turingParams.y * lap.y + xyy - conv); // Compute differential values for [u] and [v]
    bool invalDelta = any(isnan(delta)) || any(isinf(delta));
    float2 fallback = (any(isnan(gs)) || any(isinf(gs))) ? 1.0f.xx : gs; // Overwrite NaN/INF texels
    turingTex[texNdx] = invalDelta ? fallback : gs + delta; // Sacrifice accuracy in exchange for performance (ignore [dt])
                                                            // Also avoid passing NaN/INF-values into the pattern texture
}