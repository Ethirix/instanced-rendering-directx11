#include "VSOut.hlsli"

#include "Buffers/CBCamera.hlsli"
#include "Buffers/SRVInstanceData.hlsli"

struct VSIn
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Color : COLOR;
	uint Index : SV_InstanceID;
};

VSOut VS_Main(VSIn input)
{
    VSOut output = (VSOut)0;

    output.Position = float4(input.Position, 1.0f);

    output.Position = mul(output.Position, transpose(SRVInstanceData[input.Index].World));
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);

    output.Normal = input.Normal;
	output.Color = input.Color;

    return output;
}