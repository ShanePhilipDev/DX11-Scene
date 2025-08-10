#include "TerrainShader.h"

TerrainShader::TerrainShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"terrain_vs.cso", L"light_ps.cso");
}

TerrainShader::~TerrainShader()
{
	// Release the sampler state.
	if (sampleState)
	{
		sampleState->Release();
		sampleState = 0;
	}

	// Release the matrix constant buffer.
	if (matrixBuffer)
	{
		matrixBuffer->Release();
		matrixBuffer = 0;
	}

	// Release the height buffer.
	if (heightBuffer)
	{
		heightBuffer->Release();
		heightBuffer = 0;
	}

	// Release the height buffer.
	if (cameraBuffer)
	{
		cameraBuffer->Release();
		cameraBuffer = 0;
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

void TerrainShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC heightBufferDesc;
	D3D11_BUFFER_DESC cameraBufferDesc;

	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(LightShader::MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	// Setup camera buffer.
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(LightShader::CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&cameraBufferDesc, NULL, &cameraBuffer);

	// Setup height buffer.
	heightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	heightBufferDesc.ByteWidth = sizeof(HeightBufferType);
	heightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	heightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	heightBufferDesc.MiscFlags = 0;
	heightBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&heightBufferDesc, NULL, &heightBuffer);

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
	renderer->CreateSamplerState(&samplerDesc, &sampleState);
}

void TerrainShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* heightMap, float amplitude, XMMATRIX viewMatrices[LIGHT_COUNT][6], XMMATRIX projMatrices[LIGHT_COUNT][6], XMFLOAT3 camPosition)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	LightShader::MatrixBufferType* dataPtr;

	XMMATRIX world, view, proj;


	// Transpose the matrices to prepare them for the shader.
	world = XMMatrixTranspose(worldMatrix);
	view = XMMatrixTranspose(viewMatrix);
	proj = XMMatrixTranspose(projectionMatrix);

	// Set matrix buffer values as you would in the light shader.
	// Uses light shader's matrix buffer type.
	result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (LightShader::MatrixBufferType*)mappedResource.pData;
	dataPtr->world = world;// worldMatrix;
	dataPtr->view = view;
	dataPtr->projection = proj;
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			dataPtr->lightView[i][j] = XMMatrixTranspose(viewMatrices[i][j]);
			dataPtr->lightProjection[i][j] = XMMatrixTranspose(projMatrices[i][j]);
		}
	}
	deviceContext->Unmap(matrixBuffer, 0);

	// Send to vertex shader.
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	// Set height buffer values.
	HeightBufferType* heightPtr;
	deviceContext->Map(heightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	heightPtr = (HeightBufferType*)mappedResource.pData;
	heightPtr->amplitude = amplitude;
	heightPtr->padding = XMFLOAT3(0, 0, 0);
	deviceContext->Unmap(heightBuffer, 0);

	// Send to vertex shader.
	deviceContext->VSSetConstantBuffers(2, 1, &heightBuffer);

	// Set camera buffer values.
	// Uses light shader's camera buffer type.
	LightShader::CameraBufferType* cameraPtr;
	deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	cameraPtr = (LightShader::CameraBufferType*)mappedResource.pData;
	cameraPtr->cameraPosition = camPosition;
	cameraPtr->padding = 0.0f;
	deviceContext->Unmap(cameraBuffer, 0);

	// Send to vertex shader.
	deviceContext->VSSetConstantBuffers(3, 1, &cameraBuffer);

	// Set height map as shader resource, and set sampler.
	deviceContext->VSSetShaderResources(0, 1, &heightMap);
	deviceContext->VSSetSamplers(0, 1, &sampleState);
}


