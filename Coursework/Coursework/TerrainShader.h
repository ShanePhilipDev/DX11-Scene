// Shader for rendering the ground

#pragma once
#include "BaseShader.h"
#include "LightShader.h"

using namespace std;
using namespace DirectX;

class TerrainShader : public BaseShader
{
private:
	// Height of the terrain.
	struct HeightBufferType
	{
		float amplitude;
		XMFLOAT3 padding;
	};

public:
	// Constructor and destructor.
	TerrainShader(ID3D11Device* device, HWND hwnd);
	~TerrainShader();

	// Set shader's world, view and projection matrices. Heightmap texture and amplitude also included, alongside some lighting attributes to make the vertex shader compatible with the light pixel shader.
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* heightMap, float amplitude, XMMATRIX viewMatrices[LIGHT_COUNT][6], XMMATRIX projMatrices[LIGHT_COUNT][6], XMFLOAT3 camPosition);

private:
	// Initialise vertex and pixel shaders from file.
	void initShader(const wchar_t* cs, const wchar_t* ps);

private:
	// Sampler
	ID3D11SamplerState* sampleState;

	// Buffers
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* heightBuffer;
	ID3D11Buffer* cameraBuffer;
};

