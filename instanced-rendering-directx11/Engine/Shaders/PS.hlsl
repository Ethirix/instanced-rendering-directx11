#include "VSOut.hlsli"

#include "Buffers/CBCamera.hlsli"

float4 PS_Main(VSOut input) : SV_TARGET
{
	float3 ambientOut = float3(0.1f, 0.1f, 0.1f);

    float3 lightDir = normalize(-float3(-1, -1, -1));
    float diffuseIntensity = saturate(dot(input.Normal, lightDir));
    float3 diffuseOut = diffuseIntensity * float3(1.0f, 1.0f, 1.0f);

    float3 eyeDirection = normalize(CameraPosition - input.Position.xyz);
    float3 halfVector = normalize(lightDir + eyeDirection);
    float specularIntensity = pow(saturate(dot(input.Normal, halfVector)), 128);
    float3 specularOut = specularIntensity * float3(1, 1, 1) * 2;

    return float4(ambientOut * input.Color + diffuseOut * input.Color + specularOut, 1);
}