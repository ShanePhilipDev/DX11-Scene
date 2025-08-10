#include "LightShader.h"

LightShader::LightShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"light_vs.cso", L"light_ps.cso");
}


LightShader::~LightShader()
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

	// Release the layout.
	if (layout)
	{
		layout->Release();
		layout = 0;
	}

	// Release the light constant buffer.
	if (lightBuffer)
	{
		lightBuffer->Release();
		lightBuffer = 0;
	}

	// Release the camera buffer.
	if (cameraBuffer)
	{
		cameraBuffer->Release();
		cameraBuffer = 0;
	}

	//Release base shader components
	BaseShader::~BaseShader();
}



void LightShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_BUFFER_DESC cameraBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC lightBufferDesc;

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
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	// Setup the description of the dynamic camera constant buffer that is in the vertex shader.
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&cameraBufferDesc, NULL, &cameraBuffer);

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

	// Sampler for shadow map sampling.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	renderer->CreateSamplerState(&samplerDesc, &sampleStateShadow);

	// Setup the description of the light dynamic constant buffer that is in the pixel shader.
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBufferType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&lightBufferDesc, NULL, &lightBuffer);


}

void LightShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* texture, Light* lights[LIGHT_COUNT], XMFLOAT3 camPosition, LightProperties lightProperties[LIGHT_COUNT], float specularPower, ShadowMap* shadowMaps[LIGHT_COUNT][6], float shadowMapBias, XMMATRIX viewMatrices[LIGHT_COUNT][6], XMMATRIX projMatrices[LIGHT_COUNT][6], bool renderNormals, bool calculateNormals, ID3D11ShaderResourceView* heightMap, float amplitude, int resolution)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;

	XMMATRIX world, view, proj;


	// Transpose the matrices to prepare them for the shader.
	world = XMMatrixTranspose(worldMatrix);
	view = XMMatrixTranspose(viewMatrix);
	proj = XMMatrixTranspose(projectionMatrix);

	// Setup matrix buffer.
	result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = world;
	dataPtr->view = view;
	dataPtr->projection = proj;

	// Transpose and add each of the light matrices to the shader.
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			dataPtr->lightView[i][j] = XMMatrixTranspose(viewMatrices[i][j]);
			dataPtr->lightProjection[i][j] = XMMatrixTranspose(projMatrices[i][j]);
		}
	}
	deviceContext->Unmap(matrixBuffer, 0);

	// Only used in vertex shader.
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
	
	// Setup camera buffer.
	CameraBufferType* cameraPtr;
	deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	cameraPtr = (CameraBufferType*)mappedResource.pData;
	cameraPtr->cameraPosition = camPosition;
	cameraPtr->padding = 0.0f;
	deviceContext->Unmap(cameraBuffer, 0);

	// Only used in vertex shader.
	deviceContext->VSSetConstantBuffers(1, 1, &cameraBuffer);

	// Setup light buffer for the pixel shader.
	LightBufferType* lightPtr;
	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	lightPtr = (LightBufferType*)mappedResource.pData;
	XMFLOAT3 attenuation;
	bool toggle;
	int type;
	float innerCutoff;
	float outerCutoff;
	float falloff;

	// Iterate through each light and add properties to the arrays.
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		attenuation = lightProperties[i].attenuation;
		toggle = lightProperties[i].toggle;
		type = lightProperties[i].type;
		innerCutoff = lightProperties[i].innerSpotlightCutoff;
		outerCutoff = lightProperties[i].outerSpotlightCutoff;
		falloff = lightProperties[i].spotlightFalloff;

		lightPtr->ambient[i] = lights[i]->getAmbientColour();
		lightPtr->diffuse[i] = lights[i]->getDiffuseColour();
		lightPtr->position[i] = XMFLOAT4(lights[i]->getPosition().x, lights[i]->getPosition().y, lights[i]->getPosition().z, 0.0f);
		lightPtr->attenuation[i] = XMFLOAT4(attenuation.x, attenuation.y, attenuation.z, 0.0f);
		lightPtr->direction[i] = XMFLOAT4(lights[i]->getDirection().x, lights[i]->getDirection().y, lights[i]->getDirection().z, 0.0f);
		lightPtr->toggle[i] = XMINT4(toggle, toggle, toggle, toggle); // Pad by repeating toggle value.
		lightPtr->type[i] = XMINT4(type, type, type, type); // Pad by repeating type value.
		lightPtr->spotlightProperties[i] = XMFLOAT4(innerCutoff, outerCutoff, falloff, 0.0f); 
		lightPtr->specularColour[i] = lights[i]->getSpecularColour();
		lightPtr->specularPower[i] = XMFLOAT4(specularPower, specularPower, specularPower, specularPower); // Pad by repeating specular power value.
	}

	// Additional values that are not tied to each light.
	lightPtr->shadowMapBias = shadowMapBias;
	lightPtr->calculateNormals = calculateNormals;
	lightPtr->renderNormals = renderNormals;
	lightPtr->amplitude = amplitude;
	lightPtr->resolution = resolution;
	lightPtr->padding = XMFLOAT3(0, 0, 0);
	deviceContext->Unmap(lightBuffer, 0);

	// Only used in the pixel shader.
	deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

	ID3D11ShaderResourceView* shadowMapTextures[LIGHT_COUNT][6];

	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			shadowMapTextures[i][j] = shadowMaps[i][j]->getDepthMapSRV();
		}
	}

	// Set shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(0, 1, &texture);

	// Water and terrain shaders also provide a heightmap for per-pixel normal calculation.
	deviceContext->PSSetShaderResources(1, 1, &heightMap);

	// Pass shadow map texture array into the pixel shader.
	deviceContext->PSSetShaderResources(2, LIGHT_COUNT * 6, *shadowMapTextures);

	// Different samplers used for sampling textures and shadowmaps.
	deviceContext->PSSetSamplers(0, 1, &sampleState);
	deviceContext->PSSetSamplers(1, 1, &sampleStateShadow);
}
