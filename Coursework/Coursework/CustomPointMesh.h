#pragma once
#include "BaseMesh.h"

using namespace DirectX;

class CustomPointMesh : public BaseMesh
{
public:
	// Constructor and destructor
	CustomPointMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	~CustomPointMesh();

	// Use point list primitive topology.
	void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST) override;

protected:
	// Initialise buffers.
	void initBuffers(ID3D11Device* device);
};
