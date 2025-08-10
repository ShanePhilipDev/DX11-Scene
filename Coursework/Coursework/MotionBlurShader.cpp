#include "MotionBlurShader.h"

MotionBlurShader::MotionBlurShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"motionblur_vs.cso", L"motionblur_ps.cso");
}

MotionBlurShader::~MotionBlurShader()
{
	// Release the sampler state.
	if (sampleState)
	{
		sampleState->Release();
		sampleState = 0;
	}

	// Release the depth sampler state.
	if (depthSample)
	{
		depthSample->Release();
		depthSample = 0;
	}

	// Release the matrix constant buffer.
	if (matrixBuffer)
	{
		matrixBuffer->Release();
		matrixBuffer = 0;
	}

	// Release the blur buffer.
	if (blurBuffer)
	{
		blurBuffer->Release();
		blurBuffer = 0;
	}

	// Release the layout.
	if (layout)
	{
		layout->Release();
		layout = 0;
	}

	//Release base shader components
	BaseShader::~BaseShader();
}

void MotionBlurShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_BUFFER_DESC blurBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;

	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	// Setup blur buffer description.
	blurBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	blurBufferDesc.ByteWidth = sizeof(BlurBufferType);
	blurBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	blurBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	blurBufferDesc.MiscFlags = 0;
	blurBufferDesc.StructureByteStride = 0;

	// Create blur buffer.
	renderer->CreateBuffer(&blurBufferDesc, NULL, &blurBuffer);

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	renderer->CreateSamplerState(&samplerDesc, &textureSample);

	// Sampler for depth map sampling.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;

	// Create the depth sampler.
	renderer->CreateSamplerState(&samplerDesc, &depthSample);
}

void MotionBlurShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* depthTexture, ID3D11ShaderResourceView* sceneTexture, XMMATRIX viewProjInverse, XMMATRIX previousViewProj, int numSamples, float strength)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	XMMATRIX world, view, proj;


	// Transpose the matrices to prepare them for the shader.
	world = XMMatrixTranspose(worldMatrix);
	view = XMMatrixTranspose(viewMatrix);
	proj = XMMatrixTranspose(projectionMatrix);


	// Send matrix data.
	result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = world;
	dataPtr->view = view;
	dataPtr->projection = proj;

	// Transpose matrices for use in the shader.
	dataPtr->viewProjInverse = XMMatrixTranspose(viewProjInverse); 
	dataPtr->previousViewProj = XMMatrixTranspose(previousViewProj);

	deviceContext->Unmap(matrixBuffer, 0);

	// Use matrix buffer in both vertex and pixel shader.
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &matrixBuffer);

	// Set blur buffer values.
	BlurBufferType* blurPtr;
	result = deviceContext->Map(blurBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	blurPtr = (BlurBufferType*)mappedResource.pData;
	blurPtr->numSamples = numSamples;
	blurPtr->strength = strength;
	blurPtr->padding = XMFLOAT2(0, 0);
	deviceContext->Unmap(blurBuffer, 0);

	// Set blur buffer for pixel shader.
	deviceContext->PSSetConstantBuffers(1, 1, &blurBuffer);


	// Set pixel shader textures.
	deviceContext->PSSetShaderResources(0, 1, &sceneTexture);
	deviceContext->PSSetShaderResources(1, 1, &depthTexture);

	// Set pixel shader samplers.
	deviceContext->PSSetSamplers(0, 1, &sampleState);
	deviceContext->PSSetSamplers(1, 0, &depthSample);

	
}



