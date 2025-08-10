// Water vertex shader.

#define LIGHT_COUNT 4

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    
    matrix lightViewMatrix[LIGHT_COUNT][6];
    matrix lightProjectionMatrix[LIGHT_COUNT][6];
};

struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float4 colour : COLOR;
};

OutputType main(InputType input)
{
    OutputType output;
    
    // Pass the translated vertex position into the hull shader.
    output.position = mul(float4(input.position, 1), worldMatrix);
    
    // Pass the input color into the hull shader.
    output.colour = float4(1.0, 1.0, 1.0, 1.0);
    
    // Pass texture co-ordinates to the hull shader.
    output.tex = input.tex;

    return output;
}
