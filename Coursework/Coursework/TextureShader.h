// Simple shader to render a texture.
// Also capable of rendering colours over textures or just colours if the texture parameter is set to NULL.

#pragma once

#include "BaseShader.h"

using namespace std;
using namespace DirectX;

class TextureShader : public BaseShader
{
public:
	// Constructor and destructor.
	TextureShader(ID3D11Device* device, HWND hwnd);
	~TextureShader();

	// Set shader's world, view, projection, texture and colour.
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* texture, XMFLOAT4 colour);

private:
	// Initialise vertex and pixel shaders from file.
	void initShader(const wchar_t* vs, const wchar_t* ps);

	// Texture shader buffer has matrices + the colour.
	struct TextureBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMFLOAT4 colour;
	};
private:
	// Sampler
	ID3D11SamplerState* sampleState;

	// Texture
	ID3D11Buffer* textureBuffer;
	
};

