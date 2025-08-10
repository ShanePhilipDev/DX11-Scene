// Light vertex shader
// Calculate position, tex coords and normals, alongside the world position, view vector and each lights light view positions for passing into the pixel shader.

#define LIGHT_COUNT 4

// Matrix buffer
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;

    matrix lightViewMatrix[LIGHT_COUNT][6];
    matrix lightProjectionMatrix[LIGHT_COUNT][6];
};

// Camera buffer
cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition;
    float padding;
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

OutputType main(InputType input)
{
    OutputType output;
    
	// Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // Calculate the light view position for each light and face using the world matrix, the light's view matrix, and the light's projection matrix.
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

	// Calculate the normal vector against the world matrix only and normalise.
    output.normal = mul(input.normal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);

    // Calculate the position of the vertex in the world.
    output.worldPosition = mul(input.position, worldMatrix).xyz;
	
    // Calculate view vector - direction between camera and world position.
    output.viewVector = cameraPosition.xyz - output.worldPosition.xyz;
    output.viewVector = normalize(output.viewVector);

    return output;
}