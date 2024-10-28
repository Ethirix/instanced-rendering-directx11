#include "Buffers/CBCamera.hlsli"
#include "Buffers/CBObject.hlsli"

float4 VS_Main( 
	float3 pos : POSITION, 
	float3 normal : NORMAL) : SV_POSITION
{
	float4 pos4 = float4(pos, 1.0f);
	pos4 = mul(pos4, World);
	pos4 = mul(pos4, View);
	pos4 = mul(pos4, Projection);

	return pos4;
}