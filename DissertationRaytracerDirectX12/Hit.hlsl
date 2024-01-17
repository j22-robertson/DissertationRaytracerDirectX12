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

float frand(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

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

float3 getPerpendicularVector(float3 u)
{
    //abs value for positive space
    float3 a = abs(u);

    //if x is smaller than z and y return 1 else 0
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    // exclusive or, if x = 0 returns 1 and x = 1 returns 0
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    
    uint zm = 1 ^ (xm | ym);

    return cross(u, float3(xm, ym, zm));
}

float3 sampleMicrofacet(float2 bary, float roughness, float3 normal)
{
    float random = frand(bary);
    float random2 = frand(float2(bary.y, bary.x));

    float3 B = getPerpendicularVector(normal);
    float3 T = cross(B, normal);

    float a2 = roughness * roughness;

    float cosThetaH = sqrt(max(.0f, (1.0 - random) / ((a2 - 1.0) * random+1)));
    float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
    float phiH = random2 * 3.14159265359 * 2.0f;

    return T * (sinThetaH * cos(phiH)) + B * (sinThetaH * sin(phiH)) + normal * cosThetaH;

    

}

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint vertId = 3 * PrimitiveIndex();
	
	//const float3 A = float3(1, 0, 0);
	//const float3 B = float3(0, 1, 0);
	//const float3 C = float3(0, 0, 1);

    float roughness = 0.0;
    float metallic = 1.0;

    

    float3 f0 = float3(0.04, 0.04, 0.04);
	
    float3 hitColor = BTriVertex[indices[vertId + 0]].color * barycentrics.x + BTriVertex[indices[vertId + 1]].color * barycentrics.y + BTriVertex[indices[vertId + 2]].color * barycentrics.z;

    if (InstanceID() == 0)
    {
        roughness = 1.0;
        metallic =0.0;
      //  hitColor = float3(0.23, 0.7, 0.1);
    }
     hitColor = pow(hitColor, 2.2);
    f0 = lerp(f0, hitColor, float3(metallic,metallic,metallic));

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

    //bool isReflected = roughness<1.0;
    //payload.currentBounces = payload.CurrentBounces + 1;

   // payload.canReflect = isReflected * payload.canReflect * payload.maxBounces <payload.CurrentBounces;



    /// TODO: In the pipeline shader config add a new payload with reflection properties
    /// Will need to calculate the size of the struct in bytes
    /// Also requires editing raygen for max bounces, num bounces and can reflect variables in the new payload.s
   

/*
    RayDesc reflectionRay;
    ray.origin = worldorigin;
    ray.direction = normal;
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
        reflectionRay,
        payload);
        */

    float3 view_direction = normalize(-WorldRayDirection());


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

    float attenuation = 1.0 /  1.0 + 0.05 * light_dist + 0.005 * light_dist * light_dist;

    float radiance = float3(1.0, 1.0, 1.0) * attenuation;//* max(dot(halfway,centerLightDir),0.0);

    float3 lambert = hitColor/3.14159265359;

    float3 fschlick = f0 + (float3(1.0, 1.0, 1.0)-f0) * pow(1.0 - max(dot(halfway, view_direction), 0.0), 5.0);

    float D = TrowbridgeReitzD(roughness, nDotH);
        
    float G = SmithG(roughness, normal, view_direction, centerLightDir);
    
    float numerator = D * G * fschlick;
    float denominator = 4.0 * nDotV*nDotL;
    denominator = max(denominator, 0.0000001);

    float specular = numerator / denominator;

    float3 kd = float3(1.0, 1.0, 1.0) - fschlick;

    kd *= 1.0 - metallic;



    float3 reflectionColor = (1.0, 1.0, 1.0);
    if (payload.canReflect)
    {
        HitInfo reflectionPayload;
        reflectionPayload.colorAndDistance = float4(0.0, 0.0, 0.0, 0.0);

        float3 microfacet =  sampleMicrofacet(attrib.bary, roughness, normal);
        float3 reflectedDirection = reflect(-view_direction,microfacet);

        //float3 checkreflect = reflection * reflection;
        reflectionPayload.canReflect = false;
        RayDesc reflectionRay;
        reflectionRay.Origin = worldOrigin;
        reflectionRay.Direction = reflectedDirection;
        reflectionRay.TMin = 0.01;
        reflectionRay.TMax = 100000;

        TraceRay
        (SceneBVH,
            RAY_FLAG_NONE,
            0xFF,
            0,
            0,
            0,
            reflectionRay,
            reflectionPayload);
        reflectionColor = reflectionPayload.colorAndDistance.xyz;

    }


//
 //   float3 brdf = (kd * lambert + specular * reflectionColor) * radiance*nDotL;

    
    float factor = shadowPayload.ishit ? 0.3 : 1.0;
    float3 ambient = float3(0.03, 0.03, 0.03) * hitColor;
    float3 Lo = factor * (kd * lambert + specular * reflectionColor)*radiance * nDotL;
    float3 cl = Lo + ambient;
    float3 cl2 = cl / (cl + float3(1.0, 1.0, 1.0));
    float3 cl3 = pow(cl2, float3(1.0, 1.0, 1.0)/2.2);

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
