


struct HitInfo
{
	float4 colorAndDistance;
};

struct Attributes
{
	float2 bary;
};

struct STriVertex
{
	float3 vertex;
	float4 color;
};


StructuredBuffer<STriVertex> BTriVertex: register(t0);


[shader("closesthit")] void ClosestHit()
{
	//float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

//	uint vertId = 3 * PrimitiveIndex();

	
}

