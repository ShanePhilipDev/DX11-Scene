#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class FireShader : public BaseShader
{
public:
	// Constructor and destructor
	FireShader(ID3D11Device* device, HWND hwnd);
	~FireShader();

	// The fire geometry shader uses a texture, camera, the elapsed time, particle size, heights, colours and an option for rendering normals.
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* texture, Camera* camera, float elapsedTime, float particleSize, float particleHeight, float maxHeight, float minHeight, XMFLOAT4 bottomColour, XMFLOAT4 topColour, bool renderNormals);

private:
	// Camera buffer uses camera position and rotation. Used for billboarding.
	struct CameraBufferType
	{
		XMFLOAT3 cameraPosition;
		XMFLOAT2 padding;
		XMFLOAT3 cameraRotation;
	};

	// Particle buffer contains the particle's properties.
	struct ParticleBufferType
	{
		float height;
		float size;
		float elapsedTime;
		float maxHeight;
		float minHeight;
		int renderNormals;
		XMFLOAT2 padding;
		XMFLOAT4 bottomColour;
		XMFLOAT4 topColour;
	};

	// Functions for initialising shaders with a pixel shader, vertex shader and geometry shader.
	void initShader(const wchar_t* vsFilename, const wchar_t* psFilename);
	void initShader(const wchar_t* vsFilename, const wchar_t* gsFilename, const wchar_t* psFilename);

private:
	// Sampler
	ID3D11SamplerState* sampleState;

	// Buffers
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* cameraBuffer;
	ID3D11Buffer* particleBuffer;
};

