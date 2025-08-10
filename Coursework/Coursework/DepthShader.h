// Colour shader.h
// Simple shader example.
#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;


class DepthShader : public BaseShader
{

public:
	// Constructors and destructors
	DepthShader(ID3D11Device* device, HWND hwnd);
	~DepthShader();

	// Shader parameters - just world, view and projection matrices.
	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &world, const XMMATRIX &view, const XMMATRIX &projection);

private:
	// Initialise shader with vertex and pixel shader.
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	// Only one buffer - matrix buffer.
	ID3D11Buffer* matrixBuffer;
};
