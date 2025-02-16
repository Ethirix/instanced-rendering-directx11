#ifndef __VSOUT_HLSLI
#define __VSOUT_HLSLI

struct VSOut
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 Color : COLOR;
};

#endif
