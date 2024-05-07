#include "Common.hlsl"
cbuffer BackgroundColor : register(b0)
{
    float3 col;
    float padding;
}


Texture2D<float4> envmap : register(t0);

float2 wsVectorToLatLong(float3 dir)
{
    float inv_pi = 1 / 3.14159265359;
    float3 p = normalize(dir);
    float u = (1.f + atan2(p.x, -p.z) *inv_pi) * 0.5f;
    float v = acos(p.y) * inv_pi;
    return float2(u, v);
}

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    //uint2 launchIndex = DispatchRaysIndex().xy;

    //float2 dims = float2(DispatchRaysDimensions().xy);

    //float ramp = launchIndex.y / dims.y;
    
    float2 dimensions;

    envmap.GetDimensions(dimensions.x, dimensions.y);


    float2 uv = wsVectorToLatLong(WorldRayDirection());

    //envmap[uint2(uv.x * dimensions.x, uv.y * dimensions.y)].xyz,
    payload.colorAndDistance = float4(envmap[uint2(uv.x*dimensions.x, uv.y*dimensions.y)].rgb,-1.0);
}