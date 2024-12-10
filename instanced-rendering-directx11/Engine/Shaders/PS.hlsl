float4 PS_Main(float4 position : SV_POSITION) : SV_TARGET
{
    return float4(position.x / 255.0f, position.y / 255.0f, position.z / 255.0f, 1.0f);
}