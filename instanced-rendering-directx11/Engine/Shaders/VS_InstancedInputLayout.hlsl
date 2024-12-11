#include "Buffers/CBCamera.hlsli"

float4 VS_Main(
	float3 pos : POSITION,
	uint index : SV_InstanceID,
	float4 world0 : WorldMatrix0,
	float4 world1 : WorldMatrix1,
	float4 world2 : WorldMatrix2,
	float4 world3 : WorldMatrix3) : SV_POSITION
{
    float4 pos4 = float4(pos, 1.0f);

    pos4 = mul(pos4, float4x4(world0, world1, world2, world3));
    pos4 = mul(pos4, View);
    pos4 = mul(pos4, Projection);

    return pos4;
}