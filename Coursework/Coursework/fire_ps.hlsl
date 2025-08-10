// Fire pixel shader.
// Outputs either the geometry's normals or a colour based on the height of the particle.

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer ParticleBuffer : register(b0)
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
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

float4 main(InputType input) : SV_TARGET
{
    // If rendering normals is enabled, output the particle's normals.
    if(renderNormals == 1)
    {
        return float4(input.normal, 1);
    }
    else // Otherwise render a colour based on particle height.
    {
        // Lerp alpha is the difference between the particle height and max height divided by the difference between the min height and max height.
        float alpha = (particleHeight - maxHeight) / (minHeight - maxHeight);
        
        // Lerp between the colours based on this value.
        return lerp(topColour, bottomColour, alpha);
    }
    
}