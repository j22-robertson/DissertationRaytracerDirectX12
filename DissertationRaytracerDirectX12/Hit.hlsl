#include "Common.hlsl"

struct ShadowHitInfo
{
    bool ishit;
};

struct STriVertex
{
    float3 vertex;
    float4 color;
};
cbuffer Colors : register(b0)
{
    float3 A[3];
    float3 B[3];
    float3 C[3];
};


struct PerInstanceProperties
{
    float4x4 objectToWorld;
    float4x4 objectToWorldNormal;
};




RaytracingAccelerationStructure SceneBVH : register(t2);

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);
StructuredBuffer<PerInstanceProperties> perInstance : register(t3);


[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint vertId = 3 * PrimitiveIndex();
	
	//const float3 A = float3(1, 0, 0);
	//const float3 B = float3(0, 1, 0);
	//const float3 C = float3(0, 0, 1);

	
   // float3 hitColor = BTriVertex[indices[vertId + 0]].color * barycentrics.x + BTriVertex[indices[vertId + 1]].color * barycentrics.y + BTriVertex[indices[vertId + 2]].color * barycentrics.z;
  /*  float3 e1 = BTriVertex[indices[vertId + 1]].vertex - BTriVertex[indices[vertId + 0]].vertex;
    float3 e2 = BTriVertex[indices[vertId] + 2].vertex - BTriVertex[indices[vertId + 0]].vertex;
    float3 normal = normalize(cross(e2, e1));
    
    normal = mul(perInstance[InstanceID()].objectToWorldNormal, float4(normal, 0.f)).xyz;
    
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    
    float3 lightPos = float3(2, 4, -2);
    
    float3 centerLightDir = normalize(lightPos - worldOrigin);
    
    float nDotL = max(0.0f, dot(normal, centerLightDir));
    */
    
    
    float3 hitColor;
	
   if (InstanceID() < 3)
   {
        hitColor = A[InstanceID()] * barycentrics.x + B[InstanceID()] * barycentrics.y * C[InstanceID()] + barycentrics.z;
   }
    /*
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
    }*/

    //hitColor *= nDotL;

  payload.colorAndDistance = float4(hitColor, RayTCurrent());
}


[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
    
    // Hard coded light position
    float3 lightPos = float3(2, 4, -2);
    
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    
    float3 lightDir = normalize(lightPos - worldOrigin);
    
    
   
    uint vertId = 3 * PrimitiveIndex();
    
    /*
    float3 e1 = BTriVertex[vertId + 1].vertex - BTriVertex[vertId + 0].vertex;
    float3 e2 = BTriVertex[vertId + 2].vertex - BTriVertex[vertId + 0].vertex;
    float3 normal = normalize(cross(e2, e1));
    
    normal = mul(perInstance[InstanceID()].objectToWorldNormal, float4(normal, 0.0f)).xyz;
    
    
    bool isBackFacing = dot(normal, WorldRayDirection()) > 0.0f;
    
    if(isBackFacing)
    {
        normal = -normal;
    }
    
    bool isShadowed = dot(normal, lightDir) < 0.0f;
    */
   
    
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
    ray,
    shadowPayload);
    
   /* if (!isShadowed)
    {
        isShadowed = shadowPayload.ishit;
    }*/
    
    float factor = shadowPayload.ishit ? 0.3 : 1.0;
    
    //float nDotL = max(0.f, dot(normal, lightDir));
    
   
    
 
    
    
    
    
    
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    //float4 hitColor = float4(float3(0.7, 0.7, 0.3) *nDotL* factor , RayTCurrent());
    float4 hitColor = float4(float3(0.7, 0.7, 0.3) * factor, RayTCurrent());

    payload.colorAndDistance = float4(hitColor);
}
