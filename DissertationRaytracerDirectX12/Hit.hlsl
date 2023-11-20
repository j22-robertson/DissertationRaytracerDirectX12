#include "Common.hlsl"
#include "ShadowHit.hlsl"
struct STriVertex
{
    float3 vertex;
    float4 color;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);


[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint vertId = 3 * PrimitiveIndex();
	
	const float3 A = float3(1, 0, 0);
	const float3 B = float3(0, 1, 0);
	const float3 C = float3(0, 0, 1);

	
    float3 hitColor = BTriVertex[indices[vertId + 0]].color * barycentrics.x + BTriVertex[indices[vertId + 1]].color * barycentrics.y + BTriVertex[indices[vertId + 2]].color * barycentrics.z;
	
    switch (InstanceID())
    {
		case 0:
            break;
		case 1:
            hitColor = B * barycentrics.x + B * barycentrics.y + C * barycentrics.z;
            break;
		case 2:
            hitColor = C * barycentrics.x + B * barycentrics.y + C * barycentrics.z;
            break; 
    }


  payload.colorAndDistance = float4(hitColor, RayTCurrent());
}


[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
    // Hard coded light position
    float3 lightPos = float3(2, 2, 2);
    
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    
    float3 lightDir = normalize(lightPos - worldOrigin);
    
    
    RayDesc ray;
    ray.Origin = worldOrigin;
    ray.Direction = lightDir;
    ray.TMin = 0.01;
    ray.TMax = 100000;
    bool hit = true;
    
    
    ShadowHitInfo shadowPayload;
    shadowPayload.ishit= false;
    
    
    TraceRay
    (SceneBVH,
    RAY_FLAG_NONE,
    0xFF,
    1,
    0,
    1,
    shadowPayload);
    
    float factor = shadowPayload.ishit ? 0.3 : 1.0;
    
    
    
    
    
    
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    float4 hitColor = float4(float3(0.7, 0.7, 0.3) * factor, RayTCurrent());

    payload.colorAndDistance = floa4(hitColor);
}
