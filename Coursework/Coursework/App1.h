// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"
#include "WaterShader.h"
#include "PlaneTessellationMesh.h"
#include "CustomPointMesh.h"
#include "LightShader.h"
#include "DepthShader.h"
#include "TextureShader.h"
#include "TerrainShader.h"
#include "FireShader.h"
#include "MotionBlurShader.h"
#include <ctime>
#include <cmath>

// Fixed amount of lights, cubes and spheres
#define LIGHT_COUNT 4
#define CUBE_COUNT 3
#define SPHERE_COUNT 3

class App1 : public BaseApplication
{
public:

	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);
	bool frame();

	// Struct for holding the matrix transformations of objects in the scene.
	struct Object
	{
		XMFLOAT3 position;
		XMFLOAT3 scale;
		float rotationY;
	};

	// Struct that contains the specular power for different materials.
	struct SpecularValues
	{
		float water;
		float ground;
		float metal;
		float wood;
		float dog;
	};

	// Struct that contains the position and size of each fire particle.
	struct FireParticle
	{
		XMFLOAT3 position;
		float size;
	};

	// Tessellation mode for the water. It can either be tessellated based on the distance from the camera, using ImGui sliders, or using the lowest tessellation factor (called 'OFF' for simplicity).
	enum TessellationMode { DISTANCE = 0, SLIDERS, OFF };

	// Each light has a mode. They can be either directional lights, point lights or spotlights.
	enum LightMode { DIRECTIONAL = 0, POINT, SPOTLIGHT };

protected:
	// Main render function. Contains each pass and renders the final scene.
	bool render();

	// Depth pass. Used to calculate shadow maps for each light and the depth map used in the motion blur shader.
	void depthPass();

	// Render function used in the depth pass. Renders relevant objects in the scene.
	void depthRender(XMMATRIX world, XMMATRIX view, XMMATRIX projection);

	// Renders all objects in the scene with lighting and shadows.
	void scenePass();

	// Blur pass. Applies the motion blur shader to the texture generated while rendering the scene.
	void blurPass();

	// Render gui controls.
	void gui();

	// Initialise values for lights in the scene, and save the default values.
	void initLights();

	// Resets fire particles. This is done when a property of the fire is modified i.e. when the particle count is changed, or when the fire is moved.
	void resetFire();

	// Updates properties of the fire for passing into the geometry shader.
	void updateFire(float dt);

private:

	ID3D11RasterizerState* RSCullFront; // Rasterizer state that culls the front face of objects. Used for rendering the skybox.
	ID3D11RasterizerState* RSDefault; // The default rasterizer state. Used for everything else in the scene.

	// Shaders
	// *** //
	WaterShader* waterShader;
	LightShader* lightShader;
	DepthShader* depthShader;
	TextureShader* textureShader;
	TerrainShader* terrainShader;
	FireShader* fireShader;
	MotionBlurShader* motionBlurShader;
	// *** //

	// Meshes for the objects in the scene
	// *** //
	PlaneTessellationMesh* waterMesh;
	PlaneMesh* groundMesh;
	SphereMesh* sphereMesh;
	CubeMesh* cubeMesh;
	AModel* corgiMesh;
	AModel* houseMesh;
	AModel* campfireMesh;
	AModel* lampMesh;
	Model* pierMesh;
	CustomPointMesh* pointMesh;
	// *** //

	// Plane resolution (number of tiles across) for water and ground.
	int waterResolution;
	int groundResolution;

	// Objects in the scene
	// *** //
	// Don't need to rotate or scale the water or ground, so just save position.
	XMFLOAT3 waterPosition;
	XMFLOAT3 groundPosition;
	
	// Contains scale, rotation and position values.
	Object corgi;
	Object house;
	Object campfire;
	Object lamp;
	Object pier;
	Object cubes[CUBE_COUNT];
	Object spheres[SPHERE_COUNT];

	// Corgi has additional rotation variable for rotating around the fire.
	float corgiRotation;
	// *** //

	// Fire variables
	// *** //
	// Vector containing the fire particles. Each particle has a size and position.
	std::vector<FireParticle> fireParticle;

	// The number of fire particles.
	int fireParticleCount;

	// Toggle the fire on/off.
	bool fireToggle;

	// The positiion of the fire in the world.
	XMFLOAT3 firePosition;
	XMFLOAT3 previousFirePosition;

	// Defines the boundary positions of the fire in the world.
	float maxHeight;
	float minHeight;
	float maxWidth;
	float minWidth;

	// The colours of the particles at the min and max heights. Particles inbetween will have a colour between these values.
	XMFLOAT4 fireTopColour;
	XMFLOAT4 fireBottomColour;

	// Width and height of the fire.
	float fireWidth;
	float fireHeight;

	// Speed and size modifier of the particles.
	float particleSpeed;
	float particleSize;

	// The fire will change direction to simulate wind / the erraticness of fire. Timer variable to keep track of when to change direction, and how long between direction changes.
	float directionChangeTimer;
	float directionChangeTime;

	// Random position variables for the fire's direction.
	float randX;
	float randZ;
	// *** //

	

	// Motion blur variables
	// *** //
	// Shadow map object is re-used for calculating the depth map from the camera.
	ShadowMap* depthMap;

	// This texture is passed into the blur shader. The scene including lighting, particles, etc. is rendered to this texture.
	RenderTexture* sceneRenderTexture;

	// An ortho mesh for rendering the blurred scene to.
	OrthoMesh* blurOrthoMesh;

	// The view and projection matrices used in the last render. Used to calculate the position of objects in the last render without having to render the scene again.
	XMMATRIX previousView;
	XMMATRIX previousProj;

	// Toggle the blur on and off.
	bool blurToggle;

	// How many times to sample the texture during blurs.
	int blurSamples;

	// Strength modifier applied to the velocity of the blur, which is calculated using the difference in object positions between renders.
	float blurStrength;

	// Toggle blurring on fire particles. If enabled, fire particles will be rendered in the depth shader to give them a depth in the blur shader, which allows them to be blurred. 
	// Disabling this will improve performance as particles will not be rendered twice. They could still be blurred if there is an object behind them with a depth value.
	bool blurFireParticles;
	// *** //

	// Lighting variables
	// *** //
	// Array of light objects.
	Light* lights[LIGHT_COUNT];

	// Values to be passed into the light shader for each light.
	LightShader::LightProperties lightProperties[LIGHT_COUNT];
	LightShader::LightProperties defaultLightProperties[LIGHT_COUNT];

	// Booleans for rendering the light's positions and object's normals.
	bool renderLights;
	bool renderNormals;
	
	// Specular power for materials.
	SpecularValues specularValues;
	SpecularValues defaultSpecularValues;
	// *** //

	// Shadow variables
	// *** //
	// 6 shadow maps for each light. One for each face of a point light.
	ShadowMap* shadowMaps[LIGHT_COUNT][6];

	// View and projection matrices for use in lighting calculations.
	XMMATRIX viewMatrices[LIGHT_COUNT][6];
	XMMATRIX projMatrices[LIGHT_COUNT][6];

	// Ortho mesh for rendering the selected shadow map in the corner of the screen.
	OrthoMesh* shadowMapMesh;
	
	// Toggle rendering the aforementioned shadow map.
	bool renderShadowMap;

	// The light to render the shadow map from.
	int lightShadowMapToRender;

	// Which of the 6 shadow maps to render.
	int shadowMapToRender;

	// Shadow map bias applied in shadow calculations.
	float shadowMapBias;

	// Size of area to be captured in directional light shadow maps.
	int sceneSize;
	
	// Near and far cut-offs for directional light shadow maps.
	float directionalNear;
	float directionalFar;

	// Near and far cut-offs for point lights and spotlights.
	float spotPointNear;
	float spotPointFar;
	// *** //

	// Water variables
	// *** //
	// Tessellation properties
	WaterShader::TessellationProperties tessProperties;
	WaterShader::TessellationProperties defaultTessProperties;

	// Wave properties
	float waterAmplitude;
	float waterFrequency; // Not used - height map used instead.
	float waterSpeed;
	// *** //

	// Keep track of elapsed time for waves in shaders.
	float elapsedTime;

	// Amplitude to apply to the ground's heightmap.
	float terrainHeight;
};

#endif