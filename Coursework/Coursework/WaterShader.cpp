#include "WaterShader.h"



WaterShader::WaterShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"water_vs.cso", L"water_hs.cso", L"water_ds.cso", L"light_ps.cso"); 
}


WaterShader::~WaterShader()
{
	if (sampleState)
	{
		sampleState->Release();
		sampleState = 0;
	}

	if (matrixBuffer)
	{
		matrixBuffer->Release();
		matrixBuffer = 0;
	}

	if (tessellationBuffer)
	{
		tessellationBuffer->Release();
		tessellationBuffer = 0;
	}

	if (waveBuffer)
	{
		waveBuffer->Release();
		waveBuffer = 0;
	}

	if (cameraBuffer)
	{
		cameraBuffer->Release();
		cameraBuffer = 0;
	}

	if (layout)
	{
		layout->Release();
		layout = 0;
	}

	//Release base shader components
	BaseShader::~BaseShader();
}

void WaterShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{


	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(LightShader::MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create matrix buffer.
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	D3D11_BUFFER_DESC tessellationBufferDesc;
	tessellationBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	tessellationBufferDesc.ByteWidth = sizeof(TessellationBufferType);
	tessellationBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	tessellationBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	tessellationBufferDesc.MiscFlags = 0;
	tessellationBufferDesc.StructureByteStride = 0;

	renderer->CreateBuffer(&tessellationBufferDesc, NULL, &tessellationBuffer);

	D3D11_BUFFER_DESC waveBufferDesc;
	waveBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	waveBufferDesc.ByteWidth = sizeof(WaveBufferType);
	waveBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	waveBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	waveBufferDesc.MiscFlags = 0;
	waveBufferDesc.StructureByteStride = 0;

	renderer->CreateBuffer(&waveBufferDesc, NULL, &waveBuffer);

	D3D11_BUFFER_DESC cameraBufferDesc;
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;

	renderer->CreateBuffer(&cameraBufferDesc, NULL, &cameraBuffer);

	// Create a texture sampler state description.
	D3D11_SAMPLER_DESC samplerDesc;
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

void WaterShader::initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename)
{
	// InitShader must be overwritten and it will load both vertex and pixel shaders + setup buffers
	initShader(vsFilename, psFilename);

	// Load other required shaders.
	loadHullShader(hsFilename);
	loadDomainShader(dsFilename);


}


void WaterShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, TessellationProperties tessProperties, float time, float amplitude, float frequency, float speed, XMFLOAT3 camPos, XMMATRIX viewMatrices[LIGHT_COUNT][6], XMMATRIX projMatrices[LIGHT_COUNT][6], ID3D11ShaderResourceView* heightMap)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Transpose the matrices to prepare them for the shader.
	XMMATRIX world = XMMatrixTranspose(worldMatrix);
	XMMATRIX view = XMMatrixTranspose(viewMatrix);
	XMMATRIX proj = XMMatrixTranspose(projectionMatrix);

	// Write matrix information to the matrix buffer. Using light's matrix buffer type.
	result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	LightShader::MatrixBufferType* dataPtr = (LightShader::MatrixBufferType*)mappedResource.pData;
	dataPtr->world = world;
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

	// Set buffer in domain and vertex shader.
	deviceContext->DSSetConstantBuffers(0, 1, &matrixBuffer);
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	// Write tessellation information to the tessellation buffer.
	result = deviceContext->Map(tessellationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	TessellationBufferType* tessPtr = (TessellationBufferType*)mappedResource.pData;
	tessPtr->tessEdgeFactor = tessProperties.edgeFactor;
	tessPtr->tessInsideFactor = tessProperties.insideFactor;
	tessPtr->tessMode = tessProperties.mode;
	tessPtr->maxDistance = tessProperties.maxDistance;
	tessPtr->minDistance = tessProperties.minDistance;
	tessPtr->maxFactor = tessProperties.maxFactor;
	tessPtr->minFactor = tessProperties.minFactor;
	tessPtr->padding = 0.0f;
	deviceContext->Unmap(tessellationBuffer, 0);

	// Set buffer in hull shader.
	deviceContext->HSSetConstantBuffers(0, 1, &tessellationBuffer);

	// Set wave buffer values.
	result = deviceContext->Map(waveBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	WaveBufferType* wavePtr = (WaveBufferType*)mappedResource.pData;
	wavePtr->amplitude = amplitude;
	wavePtr->frequency = frequency;
	wavePtr->speed = speed;
	wavePtr->time = time;
	deviceContext->Unmap(waveBuffer, 0);

	// Set buffer in domain shader.
	deviceContext->DSSetConstantBuffers(1, 1, &waveBuffer);

	// Set camera buffer values.
	result = deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	CameraBufferType* camPtr = (CameraBufferType*)mappedResource.pData;
	camPtr->cameraPosition = camPos;
	camPtr->padding = 0.0f;
	deviceContext->Unmap(cameraBuffer, 0);

	// Camera buffer is passed to hull and domain shaders.
	deviceContext->HSSetConstantBuffers(1, 1, &cameraBuffer);
	deviceContext->DSSetConstantBuffers(2, 1, &cameraBuffer);

	// Domain shader has heightmap texture and a sampler.
	deviceContext->DSSetShaderResources(0, 1, &heightMap);
	deviceContext->DSSetSamplers(0, 1, &sampleState);
}