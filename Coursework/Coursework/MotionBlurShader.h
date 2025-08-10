#pragma once

#include "BaseShader.h"

using namespace std;
using namespace DirectX;

class MotionBlurShader : public BaseShader
{
public:
	// Constructor and destructor
	MotionBlurShader(ID3D11Device* device, HWND hwnd);
	~MotionBlurShader();

	// Pass world, view and projection matrices into shader. Also pass the scene depth map, the scene texture, the inverse view-projection matrix, the previous view-projection matrix, the number of times to sample the blur and the strength of the blur.
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* depthTexture, ID3D11ShaderResourceView* sceneTexture, XMMATRIX viewProjInverse, XMMATRIX previousViewProj, int numSamples, float strength);
private:
	// Initialise vertex and pixel shaders from file.
	void initShader(const wchar_t* vs, const wchar_t* ps);

	// Standard matrix buffer + view-projection inverse and previous view-projection for calculating world positions.
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;

		XMMATRIX viewProjInverse;
		XMMATRIX previousViewProj;
	};

	// Blur buffer includes the blur strength and the number of times to sample the texture while blurring.
	struct BlurBufferType
	{
		float strength;
		int numSamples;
		XMFLOAT2 padding;
	};

private:
	// Samplers
	ID3D11SamplerState* textureSample;
	ID3D11SamplerState* depthSample;

	// Buffers
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* blurBuffer;
};

