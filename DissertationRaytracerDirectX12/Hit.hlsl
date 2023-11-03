#include "Common.hlsl"

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
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    float3 hitColor = float3(0.7, 0.7, 0.3);

    payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
