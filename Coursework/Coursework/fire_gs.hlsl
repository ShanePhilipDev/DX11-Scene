// Fire geometry shader.
// Generates a quad that has texture co-ordinates and normals.

// Matrix buffer.
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

// Camera matrix - used for billboard calculations.
cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition;
    float2 padding;
    float3 cameraRotation;
};


cbuffer ParticleBuffer : register(b2)
{
    float particleHeight;
    float particleSize;
    float elapsedTime;
    float maxHeight;
    float minHeight;
    float renderNormals;
    float2 particlePadding;
    float4 bottomColour;
    float4 topColour;
}

cbuffer PositionBuffer
{
    // Fixed position and texture co-ordinates
    static float3 positions[4] =
    {
        float3(-particleSize, particleSize, 0),
		float3(-particleSize, -particleSize, 0),
		float3(particleSize, particleSize, 0),
		float3(particleSize, -particleSize, 0)
    };
    
    static float2 texCoords[4] =
    {
        float2(0.0, 0.0),
        float2(0.0, 1.0),
        float2(1.0, 0.0),
        float2(1.0, 1.0)
    };
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
};

[maxvertexcount(4)]
void main(point InputType input[1], inout TriangleStream<OutputType> triStream)
{
    OutputType output;
	
    
     
	
    // Move position into world space for subtracting from camera position.
    input[0].position = mul(input[0].position, worldMatrix);
    
    // For each vertex...
    for (int i = 0; i < 4; i++)
    {
        // Billboard is cylindrical and restricted to the Y axis.
        // Normals are same for each point as it's a flat quad.
        float3 normal = float3(0, 0, -1);
        
        // Calculate the vector between the vertex position and the camera's position.
        float3 posVec = float3(input[0].position.x - cameraPosition.x * 2, 0, input[0].position.z - cameraPosition.z * 2);
        
        // Calculate angle of the vector.
        float angle = atan2(posVec.x, posVec.z);
        
        // Transformation matrix for rotation.
        float3x3 rotationMatrix =
        {
            cos(angle), 0, sin(angle),
            0, 1, 0,
            -sin(angle), 0, cos(angle)
        };
        
        // Multiply the position by the rotation matrix.
        float3 newPos = mul(rotationMatrix, positions[i]);
        
        // Multiply the normal by the rotation matrix.
        normal = mul(rotationMatrix, normal);
        
        // Set position, texture co-ordinates and normal.
        output.position = input[0].position + float4(newPos, 1);
        output.position = mul(output.position, viewMatrix);
        output.position = mul(output.position, projectionMatrix);
        output.tex = texCoords[i];
        output.normal = mul(normal, (float3x3) worldMatrix);
        output.normal = normalize(output.normal);
        triStream.Append(output);
    }
    triStream.RestartStrip();
}