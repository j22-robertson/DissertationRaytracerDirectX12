cbuffer CameraParams : register(b0)
{
    float4x4 view;
    float4x4 projection;
}

struct PerInstanceProperties
{
    float4x4 objectToWorld;
    float4x4 objectToWorldNormal;
};


StructuredBuffer<PerInstanceProperties> perInstance : register(t0);

uint instanceIndex: register(b1);

struct id
{
    uint id: SV_InstanceID;
};

struct VS_INPUT
{
    float4 pos : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VS_OUTPUT main(VS_INPUT input, id Id)
{

	float4 pos = mul(perInstance[Id.id].objectToWorld, input.pos);
    pos = mul(view, pos);
    pos = mul(projection, pos);
    
    VS_OUTPUT output;

    output.pos = pos;
    output.color = input.color;
    // just pass vertex position straight through
    return output;
}
