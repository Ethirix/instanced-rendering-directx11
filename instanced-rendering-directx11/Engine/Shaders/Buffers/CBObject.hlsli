#ifndef __CBOBJECT_HLSLI__
#define __CBOBJECT_HLSLI__

cbuffer CBObject : register(b1)
{
    float4x4 World;
}

#endif