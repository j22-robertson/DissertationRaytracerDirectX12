#include "Common.hlsl"

cbuffer BackgroundColor : register(b0)
{
    float3 col;
    float padding;
}

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    uint2 launchIndex = DispatchRaysIndex().xy;

    float2 dims = float2(DispatchRaysDimensions().xy);

    float ramp = launchIndex.y / dims.y;



    payload.colorAndDistance = float4(col.x, 0.2f, 0.7f - 0.3f * ramp, -1.f);
}