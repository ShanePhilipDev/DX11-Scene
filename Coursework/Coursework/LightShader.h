#pragma once

#include "DXF.h"

#define LIGHT_COUNT 4

using namespace std;
using namespace DirectX;

class LightShader : public BaseShader
{
private:

	// Light properties passed to shader through this buffer. Arrayed items are int/float 4s that have self-contained padding so that the data is passed correctly.
	struct LightBufferType
	{
		XMFLOAT4 ambient[LIGHT_COUNT];
		XMFLOAT4 diffuse[LIGHT_COUNT];
		XMFLOAT4 position[LIGHT_COUNT];
		XMFLOAT4 attenuation[LIGHT_COUNT];
		XMFLOAT4 direction[LIGHT_COUNT];
		XMINT4 toggle[LIGHT_COUNT];
		XMINT4 type[LIGHT_COUNT];
		XMFLOAT4 spotlightProperties[LIGHT_COUNT];
		XMFLOAT4 specularColour[LIGHT_COUNT];
		XMFLOAT4 specularPower[LIGHT_COUNT];
		float shadowMapBias;
		int calculateNormals;
		int renderNormals;
		float amplitude;
		float resolution;
		XMFLOAT3 padding;
	};

	

public:
	// Constructor and destructor
	LightShader(ID3D11Device* device, HWND hwnd);
	~LightShader();

	// Light's matrix buffer. Public so other vertex shaders can use this buffer type to be compatible with the lighting pixel shader.
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;

		XMMATRIX lightView[LIGHT_COUNT][6];
		XMMATRIX lightProjection[LIGHT_COUNT][6];
	};

	// Buffer that contains the camera position.
	struct CameraBufferType
	{
		XMFLOAT3 cameraPosition;
		float padding;
	};

	// Struct for holding light properties to keep code more organised and reduce number of parameters being passed to setShaderParameters function.
	struct LightProperties
	{
		XMFLOAT3 attenuation;
		XMFLOAT3 position;
		XMFLOAT4 diffuseColour;
		XMFLOAT4 ambientColour;
		XMFLOAT4 specularColour;
		XMFLOAT3 direction;
		float innerSpotlightCutoff;
		float outerSpotlightCutoff;
		float spotlightFalloff;
		int type;
		bool toggle;
	};

	// Setup shaders with given parameters. This includes world/view/projection matrices, the texture, lights, the camera position, the light properties as listed above, the material specular power, shadow maps, shadow map bias, light view/projection matrices, a boolean for rendering normals and extra parameters for rendering manipulated geometry (normal calculation toggle, the heightmap, the amplitude used on the heightmap and the resolution of the plane).
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* texture, Light* lights[LIGHT_COUNT], XMFLOAT3 camPosition, LightProperties lightProperties[LIGHT_COUNT], float specularPower, ShadowMap* shadowMaps[LIGHT_COUNT][6], float shadowMapBias, XMMATRIX viewMatrices[LIGHT_COUNT][6], XMMATRIX projMatrices[LIGHT_COUNT][6], bool renderNormals, bool calculateNormals, ID3D11ShaderResourceView* heightMap, float amplitude, int resolution);

private:
	// Initialise shader with vertex and pixel shaders.
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	// Samplers
	ID3D11SamplerState* sampleState;
	ID3D11SamplerState* sampleStateShadow;

	// Buffers
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;
};

