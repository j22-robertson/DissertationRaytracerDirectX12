cbuffer CameraParams : register(b0)
{
    float4x4 view;
    float4x4 projection;
}





struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VS_OUTPUT main(VS_INPUT input)
{
    
    float4 pos = { input.pos.xyz, 0 };
    pos = mul(view, pos);
    pos = mul(projection, pos);
    
    VS_OUTPUT output;
    output.pos = pos;
    output.color = input.color;
    // just pass vertex position straight through
    return output;
}
