// Fire vertex shader.
// Adjust's particles position using a sine wave, then outputs the world position.

// Matrix buffer. Used here for getting the world position of the point.
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

// Particle buffer. This stage uses the elapsed time for the sine waves.
cbuffer ParticleBuffer : register(b1)
{
    float particleHeight;
    float particleSize;
    float elapsedTime;
    float maxHeight;
    float minHeight;
    int renderNormals;
    float2 particlePadding;
    float4 bottomColour;
    float4 topColour;
}

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

InputType main(InputType input)
{
    OutputType output;
    
    // Fixed values to be used in wave.
    float speed = 2;
    float frequency = 2;
    float amplitude = 0.5;
    
    // Output tex coords and normals are the same as the inputs.
    output.tex = input.tex;
    output.normal = input.normal;
    
    // Get input position.
    output.position = input.position;
    
    // Transform the x and z positions using a sine wave. The y position is adjusted outside of the shader.
    output.position.x = sin((output.position.x * frequency) + (elapsedTime * speed)) * amplitude;
    output.position.z = sin((output.position.z * frequency) + (elapsedTime * speed)) * amplitude;
    
    // Multiply output position by the world matrix to get the position in the world.
    output.position = mul(output.position, worldMatrix);
    
    return output;
}