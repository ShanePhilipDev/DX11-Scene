// Motion blur pixel shader.
// Calculates the pixel's world position and the world position in the last frame.
// The difference between these positions determines how much to blur the image.
// The amount of blur applied to objects depends on their depth - nearer objects should be blurred more.

// Textures
Texture2D SceneTexture : register(t0);
Texture2D DepthMap : register(t1);

// Samplers
SamplerState TextureSampler : register(s0);
SamplerState DepthSampler : register(s1);

// The pixel shader uses the view projection inverse and the previous view projection.
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    
    matrix viewProjInverse;
    matrix previousViewProj;
};

// Blur buffer contains strength of blur and number of times to sample the texture when blurring.
cbuffer BlurBuffer : register(b1)
{
    float strength;
    int numSamples;
    float2 padding;
};

struct InputType
{
    float4 position : SV_POSITION0;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

float4 main(InputType input) : SV_TARGET
{
    // Motion blur adapted from Nvidia's GPU Gems 3.
    // Sample the geometry depth from the depthmap.
    float depthValue = DepthMap.Sample(DepthSampler, input.tex).r; 
    
    // The viewport position - where the pixel is relative to the viewport.
    // The X and Y components are values between -1 and 1. The centre is at (0, 0).
    // The code below converts the texture co-ordinates from between 0 and 1 to between -1 and 1. The z value is the depth at which the pixel is at.
    float4 viewportPos = float4(input.tex.x * 2 - 1, (1 - input.tex.y) * 2 - 1, depthValue, 1);
    
    // Calculate the position of the pixel in the world by multiplying the viewport position by the inverted view-projection matrix, then dividing the result by its w component.
    float4 worldPos = mul(viewportPos, viewProjInverse);
    worldPos /= worldPos.w;
    
    // The current position is the position of the pixel in the viewport.
    float4 currentPos = viewportPos;
    
    // The previous position is calculated by multiplying the world position by the previous view-projection matrix, then dividing the result by its w component to get values between -1 and 1 to compare to the viewport position. 
    // We do not need to use the inverse of the previous view-projection here, as we have already translated to world-space when calculating worldPos using the current inverse view-projection matrix. We're just translating the position by the old view-projection matrix.
    float4 previousPos = mul(worldPos, previousViewProj);
    previousPos /= previousPos.w;
    
    // The velocity of the pixel is calculated by taking away the current position from the previous position divided by the strength modifier.
    // Increasing strength results in a greater velocity which increases the space between texture co-ordinates in each sample, which results in more blur.
    // When there has been no movement, the velocity will be 0 and no blurring will occur.
    float2 velocity = (previousPos - currentPos) / (20.f / strength);
    
    // Get the colour of the pixel from the scene texture.
    float4 colour = SceneTexture.Sample(TextureSampler, input.tex);
    
    // Increase the texture co-ordinates before sampling the scene again.
    input.tex += velocity;
    
    // Moving the texture co-ordinate each loop will result in adding an increasingly displaced pixel colour to the final colour.
    for (int i = 0; i < numSamples; ++i, input.tex += velocity)
    {
        colour += SceneTexture.Sample(TextureSampler, input.tex);
    }
    
    // Divide the final colour by the number of times that the texture was sampled, otherwise there will be an overly bright scene.
    return colour / (numSamples + 1);
}