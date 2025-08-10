// Depth pixel shader. Outputs the depth value.
struct InputType
{
	float4 position : SV_POSITION;
	float4 depthPosition : TEXCOORD3;
};

float4 main(InputType input) : SV_TARGET
{
	float depthValue;
    
    // Get the depth value of the pixel by dividing the Z pixel depth by the homogeneous W coordinate.
    depthValue = input.depthPosition.z / input.depthPosition.w;
    
    // Render by setting the colour to the depth value.
    return float4(depthValue, depthValue, depthValue, 1.0f);
}