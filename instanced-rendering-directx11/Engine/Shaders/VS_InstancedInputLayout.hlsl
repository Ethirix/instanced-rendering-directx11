#include "VSOut.hlsli"

#include "Buffers/CBCamera.hlsli"

struct VSIn
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Color : COLOR;
	uint Index : SV_InstanceID;
	float4 World0 : WorldMatrix0;
	float4 World1 : WorldMatrix1;
	float4 World2 : WorldMatrix2;
	float4 World3 : WorldMatrix3;
};

VSOut VS_Main(VSIn input)
{
	VSOut output = (VSOut) 0;
    output.Position = float4(input.Position, 1.0f);

    output.Position = mul(output.Position, float4x4(input.World0, input.World1, input.World2, input.World3));
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);

	output.Normal = input.Normal;
	output.Color = input.Color;

    return output;
}