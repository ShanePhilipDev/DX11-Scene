// Water shader.
// Includes vertex manipulation and tessellation.

#pragma once
#include "DXF.h"
#include "LightShader.h"

using namespace std;
using namespace DirectX;

class WaterShader : public BaseShader
{
public:
	// Constructor and destructor.
	WaterShader(ID3D11Device* device, HWND hwnd);
	~WaterShader();

	// Struct to make passing the tessellation properties in to the shader easier.
	struct TessellationProperties
	{
		float maxDistance;
		float minDistance;
		float maxFactor;
		float minFactor;
		int mode;
		XMFLOAT4 edgeFactor;
		XMFLOAT2 insideFactor;
	};

	// Pass in world, view and projection matrices. Also pass in tessellation properties, wave properties, variables for lighting and a heightmap.
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, TessellationProperties tessProperties, float time, float amplitude, float frequency, float speed, XMFLOAT3 camPos, XMMATRIX viewMatrices[LIGHT_COUNT][6], XMMATRIX projMatrices[LIGHT_COUNT][6], ID3D11ShaderResourceView* heightMap);

private:
	// Functions to initialise the shader stages from file.
	void initShader(const wchar_t* vsFilename, const wchar_t* psFilename);
	void initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename);

	// Tessellation buffer.
	struct TessellationBufferType
	{
		XMFLOAT4 tessEdgeFactor;
		XMFLOAT2 tessInsideFactor;
		int tessMode;
		float maxDistance;
		float minDistance;
		float maxFactor;
		float minFactor;
		float padding;
	};

	// Water wave buffer.
	struct WaveBufferType
	{
		float time;
		float amplitude;
		float frequency;
		float speed;
	};
	
	// Camera buffer.
	struct CameraBufferType
	{
		XMFLOAT3 cameraPosition;
		float padding;
	};

private:
	// Sampler
	ID3D11SamplerState* sampleState;

	// Buffers
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* tessellationBuffer;
	ID3D11Buffer* waveBuffer;
	ID3D11Buffer* cameraBuffer;
};

