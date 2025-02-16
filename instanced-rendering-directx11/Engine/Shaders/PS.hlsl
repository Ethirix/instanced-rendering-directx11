#include "VSOut.hlsli"

#include "Buffers/CBCamera.hlsli"

float4 PS_Main(VSOut input) : SV_TARGET
{
	float3 ambientOut = float3(0.2f, 0.2f, 0.2f);

    float3 lightDir = float3(1, 1, 1);
    float diffuseIntensity = saturate(dot(input.Normal, lightDir));
    float3 diffuseOut = diffuseIntensity * float3(0.8f, 0.8f, 0.8f);

    float3 eyeDirection = normalize(CameraPosition - input.Position.xyz);
    float3 halfVector = normalize(lightDir + eyeDirection);
    float specularIntensity = pow(saturate(dot(input.Normal, halfVector)), 32);
    float3 specularOut = specularIntensity * float3(1, 1, 1);

    return float4(input.Color + ambientOut + diffuseOut + specularOut, 1);
}