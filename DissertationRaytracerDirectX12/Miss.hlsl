

struct HitInfo
{
	float4 colorAndDistance;
};

struct Attributes
{
	float2 bary;
};

[shader("miss")] void Miss()
{
//	uint2 launchIndex = DispatchRaysIndex().xy;

//	float2 dims = float2(DispatchRaysDimensions().xy);

//	float ramp = launchIndex.y / dims.y;

	//payload.colorAndDistance = float4(0.0, 0.2f, 0.7f - 0.3f * ramp, -1.0f);
}

