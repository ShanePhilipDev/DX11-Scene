// Water Hull Shader
// Prepares control points for tessellation.

// Tessellation buffer.
cbuffer tessellationBuffer : register(b0)
{
    float4 edgeTessFactor;
    float2 insideTessFactor;
    int tessMode;
    float maxDist;
    float minDist;
    float maxTess;
    float minTess;
    float padding;
};

// Camera buffer.
cbuffer cameraBuffer : register(b1)
{
    float3 cameraPosition;
}

struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float4 colour : COLOR;
};

struct ConstantOutputType
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct OutputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float4 colour : COLOR;
};

float calculateTessFactor(float3 midpoint)
{
    // Distance from the midpoint to the camera.
    float dist = distance(midpoint, cameraPosition);
    
    // Formula used to lerp between the min and max tessellation value based on distance. Tessellation is at its highest when at min distance, and its lowest at max distance.
    float tessLerp = saturate((dist - minDist) / (maxDist - minDist));
    
    // Return lerped value.
    return lerp(maxTess, minTess, tessLerp);
}

ConstantOutputType PatchConstantFunction(InputPatch<InputType, 4> inputPatch, uint patchId : SV_PrimitiveID)
{
    ConstantOutputType output;

    if(tessMode == 0) // If using the distance tessellation mode...
    {
        // Tessellating individual patches based on distance. Adapted from Frank Luna's Introduction to 3D Game Programming with DirectX11.
        float3 edgeMidpoint[4];
        float3 patchCentre;
        
        // Midpoints of the edges are halfway between the patch positions.
        edgeMidpoint[0] = (inputPatch[0].position + inputPatch[1].position) / 2;
        edgeMidpoint[1] = (inputPatch[3].position + inputPatch[0].position) / 2;
        edgeMidpoint[2] = (inputPatch[2].position + inputPatch[3].position) / 2;
        edgeMidpoint[3] = (inputPatch[1].position + inputPatch[2].position) / 2;
        
        // Centre of the patch is the sum of all of the patch positions divided by the number of patches (4).
        patchCentre = (inputPatch[0].position + inputPatch[1].position + inputPatch[2].position + inputPatch[3].position) / 4;
        
        // Calculate the tessellation factor for each edge.
        for (int i = 0; i < 4; i++)
        {
            output.edges[i] = calculateTessFactor(edgeMidpoint[i]);
        }
            
        // Calculate tessellation factor for both inside tessellation factors. The value is the same for both.
        output.inside[0] = output.inside[1] = calculateTessFactor(patchCentre);

    }
    else if(tessMode == 1) // If using the slider mode, the user defines the tessellation factors for the edges and insides using ImGui.
    {
         // Set the tessellation factors for the four edges of the quad.
        output.edges[0] = edgeTessFactor.x;
        output.edges[1] = edgeTessFactor.y;
        output.edges[2] = edgeTessFactor.z;
        output.edges[3] = edgeTessFactor.w;

        // Set the tessellation factor for tessallating inside the quad.
        output.inside[0] = insideTessFactor.x;
        output.inside[1] = insideTessFactor.y;
    }
    else if(tessMode == 2) // Do the minimum amount of tessellation when "off".
    {
        output.edges[0] = 1;
        output.edges[1] = 1;
        output.edges[2] = 1;
        output.edges[3] = 1;
        
        output.inside[0] = 1;
        output.inside[1] = 1;
    }
    
    return output;
}

// Tessellating using quads.
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PatchConstantFunction")]
OutputType main(InputPatch<InputType, 4> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    OutputType output;


    // Set the position for this control point as the output position.
    output.position = patch[pointId].position;

    // Set the input colour as the output colour.
    output.colour = patch[pointId].colour;
    
    // Set the output tex coord as the input tex coord.
    output.tex = patch[pointId].tex;

    return output;
}
