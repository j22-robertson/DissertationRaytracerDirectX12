
//RWTexture2D<float4> gOutput : register(u0);


struct HitInfo
{
	float4 colorAndDistance;
};

struct Attributes
{
	float2 bary;
};

RaytracingAccelerationStructure SceneBVH : register(t0);



[shader("raygeneration")] void RayGen()
{

}

