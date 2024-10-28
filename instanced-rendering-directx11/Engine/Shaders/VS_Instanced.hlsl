#include "Buffers/CBCamera.hlsli"
#include "Buffers/SRVPositions.hlsli"

float4 VS_Main(
	float3 pos : POSITION,
	float3 normal : NORMAL,
	uint index : SV_InstanceID) : SV_POSITION
{
    float4 pos4 = float4(pos, 1.0f);
    pos4 = mul(pos4, SRVPositions[index]);
    pos4 = mul(pos4, View);
    pos4 = mul(pos4, Projection);

    return pos4;
}
float4 main(float4 pos : POSITION) : SV_POSITION
{
	return pos;
}