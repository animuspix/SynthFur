
cbuffer MeshTransf
{
    float4x4 view;
    float4x4 persp;
};

struct Vertex
{
    float3 pos : POSITION0;
    float3 norml : NORMAL0;
    float2 texCoord : TEXCOORD0;
};

struct Pixel
{
    float4 posProj : SV_POSITION0;
    float3 posBase : POSITION1;
    float3 normal : NORMAL0;
    float2 txCoord : TEXCOORD0;
};

Pixel main(Vertex v)
{
	// Create a returnable [Pixel]
    Pixel pixOut;

    // Project display positions, preserve reference
    // positions + normals for texturing
    pixOut.posProj = mul(mul(float4(v.pos, 1.0f), view), persp);
    pixOut.posBase = v.pos;
    pixOut.normal = v.norml;

    // Preserve input coordinates for texture output
    pixOut.txCoord = v.texCoord;

    // Pass the output [Pixel] along to the pixel shader
    return pixOut;
}