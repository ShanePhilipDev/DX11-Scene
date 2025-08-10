// Water domain shader
// After tessellation the domain shader processes the all the vertices.

#define LIGHT_COUNT 4

// Water height map and sampler.
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

// Wave buffer.
cbuffer WaveBuffer : register(b1)
{
    float time;
    float amplitude;
    float frequency;
    float speed;
};

// Camera buffer.
cbuffer CameraBuffer : register(b2)
{
    float3 cameraPosition;
    float padding;
};

struct ConstantOutputType
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float4 colour : COLOR;
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

[domain("quad")]
OutputType main(ConstantOutputType input, float2 uvwCoord : SV_DomainLocation, const OutputPatch<InputType, 4> patch)
{
    OutputType output;
   
    // Determine the position of the new vertex.  
    float3 v1 = lerp(patch[0].position, patch[1].position, uvwCoord.y);
    float3 v2 = lerp(patch[3].position, patch[2].position, uvwCoord.y);
    float3 vertexPosition = lerp(v1, v2, uvwCoord.x);

    // Move the uvw co-ordinates by time * speed in y axis.
    uvwCoord = float2(uvwCoord.x, uvwCoord.y + time * speed);
    
    // Output updated texture co-ordinates after being moved to the pixel shader.
    float2 tex1 = lerp(patch[0].tex, patch[1].tex, uvwCoord.x);
    float2 tex2 = lerp(patch[2].tex, patch[3].tex, uvwCoord.x);
    output.tex = lerp(tex1, tex2, uvwCoord.y);
    
    // Calculate height and apply to the vertex position.
    vertexPosition.y += GetHeight(heightMap.SampleLevel(heightSampler, output.tex, 0)) * amplitude;
    
    // Normals are calculated in the pixel shader for more detail.
    
    // Sin wave
    /*
    // Position
    //vertexPosition.y += sin((vertexPosition.x * frequency) + (time * speed)) * amplitude;

    // Normals
    float3 tangent = float3(1, cos((vertexPosition.x * frequency) + (time * speed)) * amplitude, 0);
    float3 bTangent = float3(0, 0, 1);
    float3 normal = cross(bTangent, tangent);
    */
  
    // Calculate the position of the new vertex against the world, view, and projection matrices.
    output.position = float4(vertexPosition, 1); // Use vertex position as already multiplied by world matrix in vertex shader.
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    output.depthPosition = output.position;
    
    // Calculate light view positions for light shader.
    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            output.lightViewPos[i][j] = float4(vertexPosition, 1);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightViewMatrix[i][j]);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightProjectionMatrix[i][j]);
        }
    }

    // Store world position for pixel shader.
    output.worldPosition = vertexPosition;
    
    // View vector for the pixel shader.
    output.viewVector = cameraPosition.xyz - output.worldPosition.xyz;
    output.viewVector = normalize(output.viewVector);

    // Send output data to light pixel shader.
    return output;
}