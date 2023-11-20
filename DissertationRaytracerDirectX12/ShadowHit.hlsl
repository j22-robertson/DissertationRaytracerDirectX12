
struct ShadowHitInfo
{
    bool ishit;
};

struct Attributes
{
    float2 uv;
};





[shader("closesthit")]
void ShadowClosestHit(inout ShadowHitInfo hit, Attributes barycentric)
{
    hit.ishit = true;
}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo hit: SV_RayPayload)
{
    hit.ishit = false;
}





