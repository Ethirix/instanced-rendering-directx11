#ifndef __CBCAMERA_HLSLI__
#define __CBCAMERA_HLSLI__

cbuffer CBCamera : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4 CameraPosition;

    float4 __Buffer1, __Buffer2, __Buffer3;
}

#endif