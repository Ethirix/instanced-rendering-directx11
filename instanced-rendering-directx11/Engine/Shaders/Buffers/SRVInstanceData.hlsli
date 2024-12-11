#ifndef __SRVINSTANCEDATA_HLSLI__
#define __SRVINSTANCEDATA_HLSLI__

struct InstanceData
{
    float4x4 World;
};

StructuredBuffer<InstanceData> SRVInstanceData : register(t0);

#endif