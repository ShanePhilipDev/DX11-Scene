#include "FireShader.h"

FireShader::FireShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"fire_vs.cso", L"fire_gs.cso", L"fire_ps.cso");
}

FireShader::~FireShader()
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

	// Release the camera buffer.
	if (cameraBuffer)
	{
		cameraBuffer->Release();
		cameraBuffer = 0;
	}
	
	// Release the particle buffer.
	if (particleBuffer)
	{
		particleBuffer->Release();
		particleBuffer = 0;
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

void FireShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	// Create a texture sampler state description.
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Setup the description of the camera buffer and create the buffer.
	D3D11_BUFFER_DESC cameraBufferDesc;
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&cameraBufferDesc, NULL, &cameraBuffer);

	// Setup the description of the particle buffer and create the buffer.
	D3D11_BUFFER_DESC particleBufferDesc;
	particleBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	particleBufferDesc.ByteWidth = sizeof(ParticleBufferType);
	particleBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	particleBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	particleBufferDesc.MiscFlags = 0;
	particleBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&particleBufferDesc, NULL, &particleBuffer);

	// Create the texture sampler state.
	renderer->CreateSamplerState(&samplerDesc, &sampleState);
}

void FireShader::initShader(const wchar_t* vsFilename, const wchar_t* gsFilename, const wchar_t* psFilename)
{
	// InitShader must be overwritten and it will load both vertex and pixel shaders + setup buffers
	initShader(vsFilename, psFilename);

	// Load other required shaders.
	loadGeometryShader(gsFilename);
}

void FireShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* texture, Camera* camera, float elapsedTime, float size, float particleHeight, float maxHeight, float minHeight, XMFLOAT4 bottomColour, XMFLOAT4 topColour, bool renderNormals)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;

	// Transpose the matrices to prepare them for the shader.
	XMMATRIX world = XMMatrixTranspose(worldMatrix);
	XMMATRIX view = XMMatrixTranspose(viewMatrix);
	XMMATRIX proj = XMMatrixTranspose(projectionMatrix);

	// Set matrix buffer values.
	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = world;
	dataPtr->view = view;
	dataPtr->projection = proj;
	deviceContext->Unmap(matrixBuffer, 0);

	// Matrix buffer used in both the vertex and geometry shader.
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
	deviceContext->GSSetConstantBuffers(0, 1, &matrixBuffer);

	// Set camera buffer values.
	CameraBufferType* cameraPtr;
	deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	cameraPtr = (CameraBufferType*)mappedResource.pData;
	cameraPtr->cameraPosition = camera->getPosition();
	cameraPtr->padding = XMFLOAT2(0, 0);
	cameraPtr->cameraRotation = camera->getRotation();
	deviceContext->Unmap(cameraBuffer, 0);

	// Used in the geometry shader.
	deviceContext->GSSetConstantBuffers(1, 1, &cameraBuffer);

	// Set particle buffer values.
	ParticleBufferType* particlePtr;
	deviceContext->Map(particleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	particlePtr = (ParticleBufferType*)mappedResource.pData;
	particlePtr->elapsedTime = elapsedTime;
	particlePtr->height = particleHeight;
	particlePtr->size = size;
	particlePtr->maxHeight = maxHeight;
	particlePtr->minHeight = minHeight;
	particlePtr->bottomColour = bottomColour;
	particlePtr->topColour = topColour;
	particlePtr->renderNormals = renderNormals;
	particlePtr->padding = XMFLOAT2(0, 0);
	deviceContext->Unmap(particleBuffer, 0);

	// Used in every stage of the shader.
	deviceContext->VSSetConstantBuffers(1, 1, &particleBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &particleBuffer);
	deviceContext->GSSetConstantBuffers(2, 1, &particleBuffer);

	// Send texture and sampler to the pixel shader.
	deviceContext->PSSetShaderResources(0, 1, &texture);
	deviceContext->PSSetSamplers(0, 1, &sampleState);
}


