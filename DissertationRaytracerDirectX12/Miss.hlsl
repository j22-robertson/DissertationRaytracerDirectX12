#include "Common.hlsl"
cbuffer BackgroundColor : register(b0)
{
    float3 col;
    float padding;
}


Texture2D<float4> envmap : register(t0);

float2 wsVectorToLatLong(float3 dir)
{
    float3 p = normalize(dir);
    float u = (1.f + atan2(p.x, -p.z) *3.14159265359) * 0.5f;
    float v = acos(p.y) * 3.14159265359;
    return float2(u, v);
}

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    uint2 launchIndex = DispatchRaysIndex().xy;

    float2 dims = float2(DispatchRaysDimensions().xy);

    float ramp = launchIndex.y / dims.y;
    
    float2 dimensions;

    envmap.GetDimensions(dimensions.x, dimensions.y);


    float2 uv = wsVectorToLatLong(WorldRayDirection());


    payload.colorAndDistance.rgb = envmap[uint2(uv * dimensions)].rgb;
   // payload.colorAndDistance.a = -1.0;
}