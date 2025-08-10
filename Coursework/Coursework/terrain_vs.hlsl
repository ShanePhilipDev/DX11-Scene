// Terrain vertex shader.
// Outputs to light's pixel shader.

#define LIGHT_COUNT 4

// Height map texture and sampler
Texture2D heightMap : register(t0);
SamplerState heightSampler : register(s0);

// Matrix buffer.
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    
    matrix lightViewMatrix[LIGHT_COUNT][6];
    matrix lightProjectionMatrix[LIGHT_COUNT][6];
};

// Height buffer.
cbuffer HeightBuffer : register(b2)
{
    float amplitude;
}

// Camera buffer.
cbuffer CameraBuffer : register(b3)
{
    float3 cameraPosition;
};

struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
    float4 depthPosition : TEXCOORD3;
    float4 lightViewPos[LIGHT_COUNT][6] : TEXCOORD4;
};

// Get the height from the heightmap's texture sample.
float GetHeight(float4 colour)
{
    float height;
    float4 textureColour = colour;
    height = textureColour.xyz;
    return height;
};

OutputType main(InputType input)
{
    OutputType output;

	// Increase the y position based on the height map's colour * the amplitude
    input.position.y += GetHeight(heightMap.SampleLevel(heightSampler, input.tex, 0)) * amplitude;
    
    // Normals are calculated in pixel shader for more detail.

	// Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    output.depthPosition = output.position;
    
    // Calculate light view positions for light shader.
    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            output.lightViewPos[i][j] = mul(input.position, worldMatrix);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightViewMatrix[i][j]);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightProjectionMatrix[i][j]);
        }
    }

	// Store the texture coordinates for the pixel shader.
    output.tex = input.tex;

    // Store world position for pixel shader.
    output.worldPosition = mul(input.position, worldMatrix).xyz;
    
    // View vector for the pixel shader.
    output.viewVector = cameraPosition.xyz - output.worldPosition.xyz;
    output.viewVector = normalize(output.viewVector);
    
    // Send output data to light pixel shader.
    return output;
}