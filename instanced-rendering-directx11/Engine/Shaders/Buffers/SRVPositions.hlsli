#ifndef __SRVPOSITIONS_HLSLI__
#define __SRVPOSITIONS_HLSLI__

struct InstanceData
{
    float4x4 Position;
};

StructuredBuffer<InstanceData> SRVPositions : register(t0);

#endif