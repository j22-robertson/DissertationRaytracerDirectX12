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
/*
cbuffer Colors : register(b0)
{
    float3 A[3];
    float3 B[3];
    float3 C[3];
};*/


struct PerInstanceProperties
{
    float4x4 objectToWorld;
    float4x4 objectToWorldNormal;
};



StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);
StructuredBuffer<PerInstanceProperties> perInstance : register(t3);

float TrowbridgeReitzD(float roughness, float NdotH)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float denom = 3.14159265 * pow(NdotH2 * (a2 - 1.0) + 1.0, 2.0);
    denom = max(denom, 0.0000001);
    return denom;
}
float SBG(float roughness, float3 N, float3 X)
{
    float r = (roughness + 1.0);
    float numerator = max(dot(N, X), 0.0);
    float k = (r * r) / 8.0;
    float denom = max(dot(N, X), 0.0) * (1.0 - k) + k;
    denom = max(denom, 0.0000001);
    return numerator / denom;
}

float SmithG(float roughness, float3 normal, float3 view_dir, float3 light_dir)
{
    return SBG(roughness, normal, view_dir) * SBG(roughness, normal, light_dir);
}


[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint vertId = 3 * PrimitiveIndex();
	
	//const float3 A = float3(1, 0, 0);
	//const float3 B = float3(0, 1, 0);
	//const float3 C = float3(0, 0, 1);

    float roughness = 0.05;
    float metallic = 1.0;

    float3 f0 = float3(0.04, 0.04, 0.04);
	
    float3 hitColor = BTriVertex[indices[vertId + 0]].color * barycentrics.x + BTriVertex[indices[vertId + 1]].color * barycentrics.y + BTriVertex[indices[vertId + 2]].color * barycentrics.z;
    
     hitColor = pow(hitColor, float3(2.2,2.2,2.2));
    f0 = lerp(f0, hitColor, metallic);

    float3 e1 = BTriVertex[indices[vertId + 1]].vertex - BTriVertex[indices[vertId + 0]].vertex;
    float3 e2 = BTriVertex[indices[vertId + 2]].vertex - BTriVertex[indices[vertId + 0]].vertex;
    
    float3 normal = normalize(cross(e2, e1));
    
    normal = mul(perInstance[InstanceID()].objectToWorldNormal, float4(normal, 0.f)).xyz;
    bool isBackFacing = dot(normal, WorldRayDirection()) > 0.0f;

    if (isBackFacing)
    {
        normal = -normal;
    }

    //normal = -normal;
    
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    
    float3 lightPos = float3(2, 10, -2);
    
    float3 centerLightDir = normalize(lightPos - worldOrigin);
    //bool isBackFacing = dot(normal, WorldRayDirection()) > 0.0f;

    bool isShadowed = dot(normal, centerLightDir);


    float3 view_direction = normalize(WorldRayOrigin());


    ShadowHitInfo shadowPayload;
    shadowPayload.ishit = false;

    RayDesc ray;
    ray.Origin = worldOrigin;
    ray.Direction = centerLightDir;
    ray.TMin = 0.01;
    ray.TMax = 100000;
    bool hit = true;

    TraceRay
    (SceneBVH,
        RAY_FLAG_NONE,
        0xFF,
        1,
        0,
        1,
        ray,
        shadowPayload);

    if (!isShadowed)
    {
        isShadowed = shadowPayload.ishit;
    }
    float3 halfway = normalize(view_direction + centerLightDir);

    float nDotL = max(0.f, dot(normal, centerLightDir));
    float nDotV = max(0.f, dot(normal, view_direction));
    float nDotH = max(0.f, dot(normal, halfway));

    float light_dist = length(lightPos - worldOrigin);

    float attenuation = 1.0 / 1.0 + (0.4 * light_dist);

    float radiance = float3(1.0, 1.0, 1.0) * attenuation * nDotL;

    float3 lambert = hitColor/3.14159265;

    float fschlick = f0 + (float3(1.0, 1.0, 1.0)-f0) * pow(1.0 - max(dot(view_direction, halfway), 0.0), 5.0);

    float D = TrowbridgeReitzD(roughness, nDotH);

    float G = SmithG(roughness, normal, view_direction, centerLightDir);
    
    float numerator = D * G * fschlick;
    float denominator = 4.0 * nDotV * nDotL;
    denominator = max(denominator, 0.000001);

    float ks = numerator / denominator;

    float3 kd = float3(1.0, 1.0, 1.0) - fschlick;

    kd *= 1.0 - metallic;

    float3 brdf = kd * lambert + ks;


    
    
    //float3 hitColor;
	
  // if (InstanceID() < 3)
   //{
     //   hitColor = A[InstanceID()] * barycentrics.x + B[InstanceID()] * barycentrics.y * C[InstanceID()] + barycentrics.z;
  // }
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
    float factor = shadowPayload.ishit ? 0.3 : 1.0;
    //float3 ambient = float3(0.03, 0.03, 0.03) * hitColor * 0.1;
    float3 Lo = brdf * radiance * nDotL *factor;
    float3 cl =   Lo;
    float3 cl2 = cl / (cl + float3(1.0, 1.0, 1.0));
    float3 cl3 = pow(cl2, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

  payload.colorAndDistance = float4(cl3, RayTCurrent());
}




[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{





    // Hard coded light position
    float3 lightPos = float3(2, 10, -2);
    
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    
    float3 lightDir = normalize(lightPos - worldOrigin);
     
   
    uint vertId = 3 * PrimitiveIndex();
    
    
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
    
    if (!isShadowed)
    {
        isShadowed = shadowPayload.ishit;
    }
    
    float factor = shadowPayload.ishit ? 0.3 : 1.0;
    
    float nDotL = max(0.f, dot(normal, lightDir));
   

    
    
    
    
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    float4 hitColor = float4(float3(0.7, 0.7, 0.3) *nDotL* factor , RayTCurrent());
    //float4 hitColor = float4(float3(0.7, 0.7, 0.3) * factor, RayTCurrent());

    payload.colorAndDistance = float4(hitColor);
}
