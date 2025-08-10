// Light pixel shader
// Handles shadows and three different kinds of lights
#define LIGHT_COUNT 4

Texture2D texture0 : register(t0);
Texture2D heightMap : register(t1);
Texture2D shadowMapTexture[LIGHT_COUNT][6] : register(t2);

SamplerState sampler0 : register(s0);
SamplerState shadowSampler : register(s1);

// Light properties
cbuffer LightBuffer : register(b0)
{
    float4 ambient[LIGHT_COUNT];
    float4 diffuse[LIGHT_COUNT];
    float4 position[LIGHT_COUNT];
    float4 attenuation[LIGHT_COUNT];
    float4 direction[LIGHT_COUNT];
    int4 toggle[LIGHT_COUNT];
    int4 type[LIGHT_COUNT];
    float4 spotlightProperties[LIGHT_COUNT];
    float4 specularColour[LIGHT_COUNT];
    float4 specularPower[LIGHT_COUNT];
    float shadowMapBias;
    int calcNormals; 
    int renderNormals;
    float amplitude;
    float resolution;
    float3 padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
    float4 depthPosition : TEXCOORD3; // not used
    float4 lightViewPos[LIGHT_COUNT][6] : TEXCOORD4;
};

// Function for calculating attenuation. Parameters for length of light vector and the light number.
float calculateAttenuation(float dist, int i)
{
    // Constant, linear and quadratic factors for attenuation.
    float con = attenuation[i].x;
    float lin = attenuation[i].y;
    float quad = attenuation[i].z;
    
    // Calculate and return the value for attenuation.
    float atten = 1 / (con + (lin * dist) + (quad * (dist * dist)));
    return atten;
}

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float4 calculateLighting(float3 lightDirection, float3 normal, float4 ldiffuse, float atten)
{
    float intensity = saturate(dot(normal, lightDirection));
    float4 colour = saturate(ldiffuse * intensity);
    return saturate(colour * atten); // Apply attenuation.
    
}

// Calculate specular lighting.
float4 calculateSpecular(float3 lightDirection, float3 normal, float3 viewVector, float4 specularColour, float specularPower)
{
    float3 halfway = normalize(lightDirection + viewVector);
    float specularIntensity = pow(max(dot(normal, halfway), 0.0f), specularPower);
    return saturate(specularColour * specularIntensity);
}

// Check if the geometry is in the shadow map.
bool hasDepthData(float2 uv)
{
    if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f)
    {
        return false;
    }
    return true;
}

bool isInShadow(Texture2D sMap, float2 uv, float4 lightViewPosition, float bias)
{
    // Sample the shadow map (get depth of geometry).
    float depthValue = sMap.Sample(shadowSampler, uv).r;
    
    // Calculate the depth from the light.
    float lightDepthValue = lightViewPosition.z / lightViewPosition.w;
    
    // Offset by shadow map bias to avoid shadow acne.
    lightDepthValue -= bias;

    // Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
    if (lightDepthValue < depthValue)
    {
        return false;
    }
    return true;
}

float2 getProjectiveCoords(float4 lightViewPosition)
{
    // Calculate the projected texture coordinates.
    float2 projTex = lightViewPosition.xy / lightViewPosition.w;
    projTex *= float2(0.5, -0.5);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

float3 calculateNormals(float2 uv)
{
    // Calculate normals for height maps - adapted from Introduction to 3D Game Programming with DirectX11 by Frank Luna
    float2 textureSize;
    heightMap.GetDimensions(textureSize.x, textureSize.y);
    
    // Calculate the size of each pixel in the texture.
    float texelCellSizeU = 1 / textureSize.x; 
    float texelCellSizeV = 1 / textureSize.y;
    
    // Dividing the texture size by a higher number will decrease the level of detail.
    //float texelCellSizeU = 3 / textureSize.x;
    //float texelCellSizeV = 3 / textureSize.y;
    
    // Get space between cells by multiplying the plane resolution by the texel size.
    float worldCellSpace = resolution * texelCellSizeU;
    
    // Texture co-ordinates to each side of the current texel position.
    float2 leftTex = uv + float2(-texelCellSizeU, 0.0f);
    float2 rightTex = uv + float2(texelCellSizeU, 0.0f);
    float2 bottomTex = uv + float2(0.0f, texelCellSizeV);
    float2 topTex = uv + float2(0.0f, -texelCellSizeV);
	
    // Get the height of the map on each side of the current position using the previously calculated texture co-ordinates and the amplitude of the height map.
    float leftY = heightMap.SampleLevel(sampler0, leftTex, 0).r * amplitude;
    float rightY = heightMap.SampleLevel(sampler0, rightTex, 0).r * amplitude;
    float bottomY = heightMap.SampleLevel(sampler0, bottomTex, 0).r * amplitude;
    float topY = heightMap.SampleLevel(sampler0, topTex, 0).r * amplitude;
    
    // Calculate the tangent and bi-tangent using the previously calculated heights and the world cell space.
    float3 tangent = normalize(float3(2 * worldCellSpace, rightY - leftY, 0.0f));
    float3 bTangent = normalize(float3(0.0f, bottomY - topY, -2.0f * worldCellSpace));

    // The normals are the cross product of the tangent and bi-tangent.
    return cross(tangent, bTangent);
}

float4 main(InputType input) : SV_TARGET
{
    // Calculate texture colour for the pixel
    float4 textureColour = texture0.Sample(sampler0, input.tex);
    
    // Variables for each light's vector and vector length
    float3 lightVector[LIGHT_COUNT]; // Light direction.
    float dist[LIGHT_COUNT]; // Distance between light and world position.
    
    // Variable for the final colour of the light for the pixel
    float4 finalColour = float4(0, 0, 0, 0);
    
    // Calculate normals for height mapped objects.
    if(calcNormals == 1)
    {
        input.normal = calculateNormals(input.tex);
    }
    
    // If the option to render normals is enabled, don't calculate lighting and output the normals as a colour instead.
    if (renderNormals == 1)
    {
        finalColour = float4(input.normal, 1);
        return finalColour;
    }
    
    // Iterate through each of the lights...
    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        if (toggle[i].x == 1) // If light is on...
        {
            // Variable for storing the colour produced by the currently iterated light.
            float4 lightColour;
            
            if (type[i].x == 0) // If it's a directional light. Calculate lighting using the light's direction and add this to the light's colour.
            {
                lightColour = ambient[i] + calculateLighting(-direction[i].xyz, input.normal, diffuse[i], 1);
            }
            else if (type[i].x == 1) // If the light is a point light, calculate lighting with the light vector and add this to the light's colour.
            {
                // Calculate light vector and distance.
                lightVector[i] = normalize(position[i].xyz - input.worldPosition);
                dist[i] = length(position[i].xyz - input.worldPosition);
                
                // Calculate lighting.
                lightColour = ambient[i] + calculateLighting(lightVector[i], input.normal, diffuse[i], calculateAttenuation(dist[i], i));
            }
            else if (type[i].x == 2) // Spotlight calculations.
            {
                // Calculate light vector and distance.
                lightVector[i] = normalize(position[i].xyz - input.worldPosition);
                dist[i] = length(position[i].xyz - input.worldPosition);
                
                // Get spotlight's properties from the light buffer.
                float innerCutoff = spotlightProperties[i].x;
                float outerCutoff = spotlightProperties[i].y;
                float falloffFactor = spotlightProperties[i].z;

                // The dot product of normalized vectors is equal to cosine of the angle. This value is between -1 and 1.
                float cosAngle = dot(normalize(-direction[i]), normalize(lightVector[i])); 

                // The spotlight's inner and outer cutoffs are equal to the cosines of each cutoff's angle. These are restricted to being between 0 and 1 using ImGui.
                
                if (cosAngle > innerCutoff) // Inner cone. When the angle of the light is greater than the inner cutoff value, the area inside is fully lit.
                {
                    lightColour = ambient[i] + calculateLighting(lightVector[i], input.normal, diffuse[i], calculateAttenuation(dist[i], i));
                }
                else if (cosAngle > outerCutoff) // Outer cone. The light's strength falls off as it gets closer to the outer cutoff. At the inner cutoff, it will be fully lit. At the outer cutoff, it will not be lit.
                {
                    float falloff = (cosAngle - outerCutoff) / (innerCutoff - outerCutoff); // Calculate how close to each cutoff the current angle is.
                    falloff = pow(falloff, falloffFactor); // Apply spotlight falloff factor. When falloff factor is 1, it will linearly interpolate between being fully lit and not being lit. Otherwise it will follow a quadratic curve.

                    lightColour = ambient[i] + calculateLighting(lightVector[i], input.normal, diffuse[i], calculateAttenuation(dist[i], i)) * falloff;
                }
                else
                {
                    // Apply only ambient lighting outside of the spotlight cutoffs.
                    lightColour = ambient[i];
                }
            }

            if (specularPower[i].x < 100) // Specular turns off at 100. When the specular power is below this value calculate the specular lighting.
            {
                // Modify texture colour by the calculated specular lighting colour. Directional lights use the light's direction, point and spotlights use the light vector.
                if (type[i].x == 0) 
                {
                    textureColour = textureColour + calculateSpecular(-direction[i].xyz, input.normal, input.viewVector, specularColour[i], specularPower[i].x);
                }
                else
                {
                    textureColour = textureColour + saturate(calculateSpecular(lightVector[i], input.normal, input.viewVector, specularColour[i], specularPower[i].x));
                }
            }

            // Saturate light colour so that diffuse and ambient don't exceed the max RGB values. Light colour is clamped as the attenuation value could reach very high values. When multiplying light by attenuation, this can cause the colour to change.
            lightColour = saturate(clamp(lightColour, float4(0, 0, 0, 0), diffuse[i] + ambient[i]));
            
            // Shadows
            // If the light is a directional light or spotlight...
            if (type[i].x == 0 || type[i].x == 2)
            {
                // Calculate the projected texture coordinates using the light view position of the first face.
                float2 pTexCoord = getProjectiveCoords(input.lightViewPos[i][0]);

                // If location is within the shadowmap.
                if (hasDepthData(pTexCoord))
                {
                    // Check if the point is in a shadow or not.
                    if (isInShadow(shadowMapTexture[i][0], pTexCoord, input.lightViewPos[i][0], shadowMapBias))
                    {
                        // If it is, only add the ambient lighting to the final colour.
                        finalColour += ambient[i];
                    }
                    else
                    {
                        // If it isn't, add the full lighting value to the final colour.
                        finalColour += lightColour;
                    }
                }
                else // Apply lighting outside of area where shadows are calculated. These objects will be lit but cast no shadows.
                {
                    finalColour += lightColour;
                }
            }
            else if (type[i].x == 1) // If the light is a point light...
            {
                // Iterate through each direction's shadow map.
                for (int j = 0; j < 6; j++)
                {
                    // Calculate the projected texture coordinates using the light view position of the first face.
                    float2 pTexCoord = getProjectiveCoords(input.lightViewPos[i][j]);
                    
                    // If location is within the shadowmap.
                    if (hasDepthData(pTexCoord))
                    {
                        if (isInShadow(shadowMapTexture[i][j], pTexCoord, input.lightViewPos[i][j], shadowMapBias))
                        {
                            // If it is, only add the ambient lighting to the final colour.
                            finalColour += ambient[i];
                        }
                        else
                        {
                            // If it isn't, add the full lighting value to the final colour.
                            finalColour += lightColour;
                        }
                    }
                }
            }
        } 
    }
   
    // Multiply the light colour by the texture's colour to get the final colour of the pixel.
    finalColour *= textureColour;
    
    // Return the final pixel colour.
    return finalColour;
}



