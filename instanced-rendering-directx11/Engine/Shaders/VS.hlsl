#include "VSOut.hlsli"

#include "Buffers/CBCamera.hlsli"
#include "Buffers/CBObject.hlsli"

struct VSIn
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Color : COLOR;
};

VSOut VS_Main(VSIn input)
{
	VSOut output = (VSOut) 0;

	output.Position = float4(input.Position, 1.0f);
    output.Position = mul(output.Position, transpose(World));
	output.Position = mul(output.Position, View);
	output.Position = mul(output.Position, Projection);

	output.Normal = input.Normal;
	output.Color = input.Color;

	return output;
}