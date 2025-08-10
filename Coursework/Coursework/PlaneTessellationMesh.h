// Plane tessellation mesh
// Adapted from framework's PlaneMesh

#pragma once
#include "BaseMesh.h"

using namespace DirectX;

class PlaneTessellationMesh : public BaseMesh
{
public:
	PlaneTessellationMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int res = 100);
	~PlaneTessellationMesh();

	// Topology set for handling quad tessellation.
	void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST) override;
protected:
	void initBuffers(ID3D11Device* device);
	int resolution;
};

