#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_SIMPLE
#include <d2d1effecthelpers.hlsli>


float4 InvertPS(float2 uv : D2D_INPUT_POSITION) : SV_Target
{
    float4 color = D2DGetInput(0, uv);

    color.rgb = 1.0 - color.rgb;

    return color;
}