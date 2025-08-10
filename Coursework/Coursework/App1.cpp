// CMP301 Coursework - Shane Philip - 1902474
#include "App1.h"

App1::App1()
{

}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);
	
	// Save default rasterizer state.
	renderer->getDeviceContext()->RSGetState(&RSDefault);

	// Create new rasterizer state for culling front faces.
	D3D11_RASTERIZER_DESC rastDesc;
	rastDesc.CullMode = D3D11_CULL_NONE;
	renderer->getDevice()->CreateRasterizerState(&rastDesc, &RSCullFront);
	
	// Initialising shaders
	// *** //
	waterShader = new WaterShader(renderer->getDevice(), hwnd);
	lightShader = new LightShader(renderer->getDevice(), hwnd);
	depthShader = new DepthShader(renderer->getDevice(), hwnd);
	textureShader = new TextureShader(renderer->getDevice(), hwnd);
	terrainShader = new TerrainShader(renderer->getDevice(), hwnd);
	fireShader = new FireShader(renderer->getDevice(), hwnd);
	motionBlurShader = new MotionBlurShader(renderer->getDevice(), hwnd);
	// *** //

	// Initialising meshes
	// *** //
	waterResolution = 50;
	groundResolution = 100;

	waterMesh = new PlaneTessellationMesh(renderer->getDevice(), renderer->getDeviceContext(), waterResolution);
	groundMesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), groundResolution);
	sphereMesh = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	cubeMesh = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
	corgiMesh = new AModel(renderer->getDevice(), "res/corgi2.obj");
	houseMesh = new AModel(renderer->getDevice(), "res/house.obj");
	campfireMesh = new AModel(renderer->getDevice(), "res/campfire.obj");
	lampMesh = new AModel(renderer->getDevice(), "res/lamp.obj");
	pierMesh = new Model(renderer->getDevice(), renderer->getDeviceContext(), "res/pier.obj");
	pointMesh = new CustomPointMesh(renderer->getDevice(), renderer->getDeviceContext());
	shadowMapMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), 256, 256, screenWidth * 0.35, screenHeight * 0.25); // 256x256 pixels in top right corner
	// *** //
	

	// Load textures
	// *** //
	textureMgr->loadTexture(L"brick", L"res/brick1.dds");
	textureMgr->loadTexture(L"height", L"res/heightmap.png");
	textureMgr->loadTexture(L"sky", L"res/sky.jpg");
	textureMgr->loadTexture(L"water_height", L"res/water_heightmap.png");
	textureMgr->loadTexture(L"water", L"res/water.jpg");
	textureMgr->loadTexture(L"grass", L"res/grass.jpg");
	textureMgr->loadTexture(L"corgi", L"res/corgi2black.png");
	textureMgr->loadTexture(L"house", L"res/house.jpg");
	textureMgr->loadTexture(L"campfire", L"res/campfire.png");
	textureMgr->loadTexture(L"metal", L"res/metal.jpg");
	textureMgr->loadTexture(L"wood", L"res/wood.png");
	// *** //

	// Set amplitude of heightmap. Heightmap has flat surfaces, hills and a lake in the centre. Also used to adjust heightmap's y position.
	terrainHeight = 30;

	// Set start time to 0.
	elapsedTime = 0;

	// Set starting positions, scale and rotation.
	// *** //
	waterPosition = XMFLOAT3(-25, -2, -25);
	groundPosition = XMFLOAT3(-50, -terrainHeight / 2, -50);

	corgi.position = XMFLOAT3(3, 0, 3);
	house.position = XMFLOAT3(0, 0, 25);
	campfire.position = XMFLOAT3(-35, 0, -5);
	lamp.position = XMFLOAT3(30, -1, 0);
	pier.position = XMFLOAT3(0, -1, 10);

	corgi.scale = XMFLOAT3(1, 1, 1);
	house.scale = XMFLOAT3(0.3, 0.3, 0.3);
	campfire.scale = XMFLOAT3(0.4, 0.4, 0.4);
	lamp.scale = XMFLOAT3(1, 1, 1);
	pier.scale = XMFLOAT3(0.2, 0.2, 0.2);

	corgi.rotationY = XMConvertToRadians(45);
	house.rotationY = XMConvertToRadians(90);
	campfire.rotationY = XMConvertToRadians(0);
	lamp.rotationY = XMConvertToRadians(90);
	pier.rotationY = XMConvertToRadians(-60);
	
	for (int i = 0; i < CUBE_COUNT; i++)
	{
		cubes[i].position = XMFLOAT3(27, 1, 2 + (i * - 2));
		cubes[i].scale = XMFLOAT3(0.5, 0.5, 0.5);
		cubes[i].rotationY = XMConvertToRadians(0);
	}

	for (int i = 0; i < SPHERE_COUNT; i++)
	{
		spheres[i].position = XMFLOAT3(25, 1, 2 + (i * - 2));
		spheres[i].scale = XMFLOAT3(0.5, 0.5, 0.5);
		spheres[i].rotationY = XMConvertToRadians(0);
	}
	// *** //

	// Intialising shadow maps
	// *** //
	int shadowmapWidth = 2048;
	int shadowmapHeight = 2048;

	renderShadowMap = false;
	lightShadowMapToRender = 1;
	shadowMapToRender = 1;
	sceneSize = 100; // Scene size of 100 encompasses the whole map when light is directly above. Reducing scene size increases quality of shadows but doesn't cover the whole map.
	shadowMapBias = 0.005f;
	directionalNear = 0.1f;
	directionalFar = 75.0f;
	spotPointNear = 1.0f;
	spotPointFar = 100.0f;

	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			shadowMaps[i][j] = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);
		}
	}
	// *** //

	// Initialise lights.
	initLights();

	// Set up motion blur variables.
	// *** //
	depthMap = new ShadowMap(renderer->getDevice(), screenWidth, screenHeight);
	blurOrthoMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), screenWidth, screenHeight);
	sceneRenderTexture = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);

	blurToggle = true; // On by default
	blurStrength = 1.5;
	blurSamples = 4;
	blurFireParticles = true;
	// *** //

	// Set up fire variables.
	// *** //
	fireToggle = true; // On by default

	// Align fire with campfire model
	XMFLOAT3 campfireOffset(0, -1, 0);
	firePosition = XMFLOAT3(lights[1]->getPosition().x + campfireOffset.x, lights[1]->getPosition().y + campfireOffset.y, lights[1]->getPosition().z + campfireOffset.z);
	previousFirePosition = firePosition; // Previous position is same as actual position at start.

	particleSize = 1;
	particleSpeed = 0.5;

	fireBottomColour = XMFLOAT4(1, 0.2, 0, 1);
	fireTopColour = XMFLOAT4(1, 1, 0, 1);

	fireParticleCount = 1000;

	directionChangeTimer = 0;
	directionChangeTime = 0.5; // Change every 0.5 seconds.

	fireHeight = 3;
	fireWidth = 0.6;

	maxHeight = fireHeight;
	minHeight = 0;
	maxWidth = fireWidth;
	minWidth = -fireWidth;

	// Seed random number generator with time. Used for randomising particle positions.
	srand(time(0));

	// Generate fire using initialised variables.
	resetFire();
	// *** //

	// Setup water variables.
	// *** //
	// Properties kept in struct for passing through to shader easily. Struct is defined in the shader class itself.
	tessProperties.mode = TessellationMode::DISTANCE;
	tessProperties.edgeFactor = XMFLOAT4(4, 4, 4, 4);
	tessProperties.insideFactor = XMFLOAT2(4, 4);
	tessProperties.maxDistance = 30;
	tessProperties.minDistance = 0;
	tessProperties.maxFactor = 10;
	tessProperties.minFactor = 1;

	defaultTessProperties = tessProperties; // Save properties for reverting back to default when ImGui button is pressed.

	waterAmplitude = 0.4;
	waterFrequency = 0.4;
	waterSpeed = 0.4;
	// *** // 
}

void App1::initLights()
{
	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
	XMFLOAT3 position;
	XMFLOAT3 direction;
	XMFLOAT4 specular;

	// Not rendering light position or object normals by default.
	renderNormals = false;
	renderLights = false;

	// Initialise all lights with default values. Some lights will overwrite these defaults.
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		// Create light
		lights[i] = new Light();

		// Default light is a white point light above centre with some attenuation. Direction is down if changed to spotlight or directional light.
		defaultLightProperties[i].toggle = false;
		defaultLightProperties[i].type = LightMode::POINT;
		defaultLightProperties[i].attenuation = XMFLOAT3(0.5f, 0.125f, 0.0f);
		defaultLightProperties[i].position = XMFLOAT3(0.0f, 10.0f, 0.0f);
		defaultLightProperties[i].diffuseColour = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		defaultLightProperties[i].ambientColour = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		defaultLightProperties[i].direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
		defaultLightProperties[i].outerSpotlightCutoff = 0.6;
		defaultLightProperties[i].innerSpotlightCutoff = 0.7;
		defaultLightProperties[i].spotlightFalloff = 1.0f;
		defaultLightProperties[i].specularColour = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

		// Setup properties.
		lightProperties[i] = defaultLightProperties[i];

		// Extract ambient, diffuse, position, direction and specular colour properties, and apply them using light's setter functions.
		ambient = lightProperties[i].ambientColour;
		diffuse = lightProperties[i].diffuseColour;
		position = lightProperties[i].position;
		direction = lightProperties[i].direction;
		specular = lightProperties[i].specularColour;

		lights[i]->setAmbientColour(ambient.x, ambient.y, ambient.z, ambient.w);
		lights[i]->setDiffuseColour(diffuse.x, diffuse.y, diffuse.z, diffuse.w);
		lights[i]->setPosition(position.x, position.y, position.z);
		lights[i]->setDirection(direction.x, direction.y, direction.z);
		lights[i]->setSpecularColour(specular.x, specular.y, specular.z, specular.w);

		// Generate ortho and projection matrices for the light.
		lights[i]->generateOrthoMatrix((float)sceneSize, (float)sceneSize, directionalNear, directionalFar);
		lights[i]->generateProjectionMatrix(spotPointNear, spotPointFar);
	}

	// Light 1 - Main Light
	// Grey directional light with slight angle for lighting the whole scene. Also produces some grey ambient light.
	// *** //
	defaultLightProperties[0].toggle = true;
	defaultLightProperties[0].type = LightMode::DIRECTIONAL;
	defaultLightProperties[0].ambientColour = XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f);
	defaultLightProperties[0].diffuseColour = XMFLOAT4(0.35f, 0.35f, 0.35f, 1.0f);
	defaultLightProperties[0].direction = XMFLOAT3(0.1f, -1.0f, -0.1f);
	
	lightProperties[0] = defaultLightProperties[0];

	ambient = lightProperties[0].ambientColour;
	diffuse = lightProperties[0].diffuseColour;
	position = lightProperties[0].position;
	direction = lightProperties[0].direction;
	specular = lightProperties[0].specularColour;

	lights[0]->setAmbientColour(ambient.x, ambient.y, ambient.z, ambient.w);
	lights[0]->setDiffuseColour(diffuse.x, diffuse.y, diffuse.z, diffuse.w);
	lights[0]->setPosition(position.x, position.y, position.z);
	lights[0]->setDirection(direction.x, direction.y, direction.z);
	lights[0]->setSpecularColour(specular.x, specular.y, specular.z, specular.w);
	// *** //

	// Light 2 - Fire
	// Orange point light placed above the campfire.
	// *** //
	defaultLightProperties[1].toggle = true;
	defaultLightProperties[1].type = LightMode::POINT;
	defaultLightProperties[1].diffuseColour = XMFLOAT4(1.0f, 0.3f, 0.0f, 1.0f);
	defaultLightProperties[1].specularColour = XMFLOAT4(1.0f, 0.3f, 0.0f, 1.0f);
	defaultLightProperties[1].position = XMFLOAT3(campfire.position.x, campfire.position.y + 1, campfire.position.z);
	defaultLightProperties[1].attenuation = XMFLOAT3(0.25, 0.25, 0.0);

	lightProperties[1] = defaultLightProperties[1];
	
	ambient = lightProperties[1].ambientColour;
	diffuse = lightProperties[1].diffuseColour;
	position = lightProperties[1].position;
	direction = lightProperties[1].direction;
	specular = lightProperties[1].specularColour;

	lights[1]->setAmbientColour(ambient.x, ambient.y, ambient.z, ambient.w);
	lights[1]->setDiffuseColour(diffuse.x, diffuse.y, diffuse.z, diffuse.w);
	lights[1]->setPosition(position.x, position.y, position.z);
	lights[1]->setDirection(direction.x, direction.y, direction.z);
	lights[1]->setSpecularColour(specular.x, specular.y, specular.z, specular.w);
	// *** //

	// Light 3 - Lamp
	// Blue spotlight placed on street lamp.
	// *** //
	defaultLightProperties[2].toggle = true;
	defaultLightProperties[2].type = LightMode::SPOTLIGHT;
	defaultLightProperties[2].diffuseColour = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	defaultLightProperties[2].specularColour = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	defaultLightProperties[2].direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	defaultLightProperties[2].position = XMFLOAT3(lamp.position.x - 1.5, lamp.position.y + 9, lamp.position.z);
	defaultLightProperties[2].attenuation = XMFLOAT3(0.1, 0.1, 0.0);
	defaultLightProperties[2].outerSpotlightCutoff = 0.85;
	defaultLightProperties[2].innerSpotlightCutoff = 0.95;

	lightProperties[2] = defaultLightProperties[2];

	ambient = lightProperties[2].ambientColour;
	diffuse = lightProperties[2].diffuseColour;
	position = lightProperties[2].position;
	direction = lightProperties[2].direction;
	specular = lightProperties[2].specularColour;

	lights[2]->setAmbientColour(ambient.x, ambient.y, ambient.z, ambient.w);
	lights[2]->setDiffuseColour(diffuse.x, diffuse.y, diffuse.z, diffuse.w);
	lights[2]->setPosition(position.x, position.y, position.z);
	lights[2]->setDirection(direction.x, direction.y, direction.z);
	lights[2]->setSpecularColour(specular.x, specular.y, specular.z, specular.w);
	// *** //

	// Set specular power for each material.
	defaultSpecularValues.water = 5;
	defaultSpecularValues.ground = 100;
	defaultSpecularValues.dog = 100;
	defaultSpecularValues.wood = 75;
	defaultSpecularValues.metal = 25;

	// Save for resetting.
	specularValues = defaultSpecularValues;
}

void App1::resetFire()
{
	// Empty the vector.
	fireParticle.clear();

	// Loop that repeats for the user defined amount of particles.
	// It creates a particle, adds it to the vector, then generates a random position for the particle and sets its size.
	for (int i = 0; i < fireParticleCount; i++)
	{
		FireParticle particle;
		fireParticle.push_back(particle);

		float x = firePosition.x + static_cast<float> (rand()) / static_cast <float> (RAND_MAX / (fireWidth * 2)) - fireWidth;
		float y = firePosition.y + static_cast<float> (rand()) / static_cast <float> (RAND_MAX / (fireHeight));
		float z = firePosition.z + static_cast<float> (rand()) / static_cast <float> (RAND_MAX / (fireWidth * 2)) - fireWidth;
		fireParticle[i].position = XMFLOAT3(x, y, z);
		fireParticle[i].size = particleSize;
	}
	
}

// Function to lerp between 2 values.
float lerp(float v0, float v1, float t)
{
	return v0 + t * (v1 - v0);
};

void App1::updateFire(float dt)
{
	// Increment timer by delta time.
	directionChangeTimer += dt;

	// Position fire.
	XMFLOAT3 campfireOffset(0, -1, 0);
	firePosition = XMFLOAT3(lights[1]->getPosition().x + campfireOffset.x, lights[1]->getPosition().y + campfireOffset.y, lights[1]->getPosition().z + campfireOffset.z);

	// Reset fire when position changes.
	if (firePosition.x != previousFirePosition.x || firePosition.y != previousFirePosition.y || firePosition.z != previousFirePosition.z)
	{
		previousFirePosition = firePosition;
		resetFire();
	}

	// Calculate fire boundaries.
	maxHeight = firePosition.y + fireHeight;
	minHeight = firePosition.y;
	maxWidth = fireWidth;
	minWidth = -fireWidth;

	// If direction exceeds change time, generate a new random X and Z value for the fire's top position.
	if (directionChangeTimer > directionChangeTime)
	{
		directionChangeTimer -= directionChangeTime;
		randX = minWidth + static_cast<float> (rand()) / static_cast <float> (RAND_MAX / (fireWidth * 2));
		randZ = minWidth + static_cast<float> (rand()) / static_cast <float> (RAND_MAX / (fireWidth * 2));
	}

	// Position of the top of the fire. Particles move towards this position.
	XMFLOAT3 top = XMFLOAT3(firePosition.x + randX, maxHeight + 1, firePosition.z + randZ);

	// Loops through every particle.
	for (int i = 0; i < fireParticleCount; i++)
	{
		// Calculate direction by taking the particle position away from the top position.
		XMFLOAT3 direction;
		direction.x = top.x - fireParticle[i].position.x;
		direction.y = top.y - fireParticle[i].position.y;
		direction.z = top.z - fireParticle[i].position.z;

		if (dt < 0.1) // Don't calculate new position if dt is too big, such as when screen is being moved as this can break the fire.
		{
			// Move particle in direction multiplied by speed and delta time.
			fireParticle[i].position.x += direction.x * dt * particleSpeed;
			fireParticle[i].position.y += direction.y * dt * particleSpeed;
			fireParticle[i].position.z += direction.z * dt * particleSpeed;
		}

		// When a particle reaches the max height, move it to the bottom and generate new x and z positions.
		if (fireParticle[i].position.y > maxHeight)
		{
			fireParticle[i].position.x = firePosition.x + static_cast<float> (rand()) / static_cast <float> (RAND_MAX / (fireWidth * 2)) - fireWidth;
			fireParticle[i].position.y = firePosition.y;
			fireParticle[i].position.z = firePosition.z + static_cast<float> (rand()) / static_cast <float> (RAND_MAX / (fireWidth * 2)) - fireWidth;
		}

		// Factor for lerping the particle's size based on the distance to the top.
		float y = (fireParticle[i].position.y - maxHeight) / (minHeight - maxHeight);

		// Interpolate particle size.
		fireParticle[i].size = lerp(0, 0.2, y) * particleSize;
	}
	
}

App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();
}


bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}
	
	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	return true;
}

void App1::depthPass()
{
	// This function goes through every light that is turned on, and generates shadowmaps for them. Also generates a depth map for the motion blur.
	XMMATRIX lightViewMatrix;
	XMMATRIX lightProjectionMatrix;
	XMMATRIX cameraViewMatrix;
	XMMATRIX cameraProjectionMatrix;
	XMMATRIX worldMatrix;

	// Iterate through each light.
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		// If the light is on...
		if (lightProperties[i].toggle)
		{
			if (lightProperties[i].type == LightMode::DIRECTIONAL)
			{
				// Set the render target to be the shadowmap.
				// As it is a directional light, only use the first shadowmap for this light.
				shadowMaps[i][0]->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

				// Generate view and ortho matrix for light.
				lights[i]->generateViewMatrix();
				lights[i]->generateOrthoMatrix((float)sceneSize, (float)sceneSize, directionalNear, directionalFar);

				// Set view, projection and world matrices.
				lightViewMatrix = lights[i]->getViewMatrix();
				lightProjectionMatrix = lights[i]->getOrthoMatrix(); // Ortho matrix is used with directional shadows.
				worldMatrix = renderer->getWorldMatrix();

				// Save the view and projection matrices for future use.
				viewMatrices[i][0] = lightViewMatrix;
				projMatrices[i][0] = lightProjectionMatrix;

				// Render the scene using the generated matrices.
				depthRender(worldMatrix, lightViewMatrix, lightProjectionMatrix);

				// Set back buffer as render target and reset view port.
				renderer->setBackBufferRenderTarget();
				renderer->resetViewport();
			}
			else if (lightProperties[i].type == LightMode::SPOTLIGHT)
			{
				// Set the render target to be the shadowmap.
				// Spotlights also only use the first shadowmap.
				shadowMaps[i][0]->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

				// Generates view and projection matrices for the spotlight, using the user-defined near and far cut-offs.
				lights[i]->generateViewMatrix();
				lights[i]->generateProjectionMatrix(spotPointNear, spotPointFar);

				// Set view, projection and world matrices.
				lightViewMatrix = lights[i]->getViewMatrix();
				lightProjectionMatrix = lights[i]->getProjectionMatrix(); // Projection matrix is used for spotlights.
				worldMatrix = renderer->getWorldMatrix();

				// Save the view and projection matrices for future use.
				viewMatrices[i][0] = lightViewMatrix;
				projMatrices[i][0] = lightProjectionMatrix;

				// Render the scene using the generated matrices.
				depthRender(worldMatrix, lightViewMatrix, lightProjectionMatrix);

				// Set back buffer as render target and reset view port.
				renderer->setBackBufferRenderTarget();
				renderer->resetViewport();
			}
			else if (lightProperties[i].type == LightMode::POINT)
			{
				// Point lights have 6 shadow maps - one for each direction.
				// For each shadow map...
				for (int j = 0; j < 6; j++)
				{
					// Set the render target to be the shadowmap.
					shadowMaps[i][j]->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

					// Set the direction based on the loop iteration.
					switch (j)
					{
					case 0:
						lights[i]->setDirection(0, -1, 0); // Down
						break;
					case 1:
						lights[i]->setDirection(0, 1, 0); // Up
						break;
					case 2:
						lights[i]->setDirection(0, 0, -1); // Backwards
						break;
					case 3:
						lights[i]->setDirection(0, 0, 1); // Forwards
						break;
					case 4:
						lights[i]->setDirection(-1, 0, 0); // Left
						break;
					case 5:
						lights[i]->setDirection(1, 0, 0); // Right
						break;
					}

					// Generates view and projection matrices for this point light direction, using the user-defined near and far cut-offs.
					lights[i]->generateViewMatrix();
					lights[i]->generateProjectionMatrix(spotPointNear, spotPointFar);

					// Set view, projection and world matrices.
					lightViewMatrix = lights[i]->getViewMatrix();
					lightProjectionMatrix = lights[i]->getProjectionMatrix();
					worldMatrix = renderer->getWorldMatrix();

					// Save the view and projection matrices for future use.
					viewMatrices[i][j] = lightViewMatrix;
					projMatrices[i][j] = lightProjectionMatrix;

					// Render the scene using the generated matrices.
					depthRender(worldMatrix, lightViewMatrix, lightProjectionMatrix);

					// Set back buffer as render target and reset view port.
					renderer->setBackBufferRenderTarget();
					renderer->resetViewport();
				}

				// Reset light direction back to what it was before generating point shadowmaps.
				XMFLOAT3 direction = lightProperties[i].direction;
				lights[i]->setDirection(direction.x, direction.y, direction.z);
			}
		}
	}
	
	// Generate a depth map for the motion blur.
	// Use camera view matrix.
	cameraViewMatrix = camera->getViewMatrix();
	cameraProjectionMatrix = renderer->getProjectionMatrix();
	worldMatrix = renderer->getWorldMatrix();

	// Set the render target to be the depth map.
	depthMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

	// Render scene from the camera's perspective.
	depthRender(worldMatrix, cameraViewMatrix, cameraProjectionMatrix);

	// Render fire particles to the depth map. This is done outside of the main depth render function so that it doesn't occur during shadow mapping. If particles cast shadows, they would be rendered up to 130000 times a frame (24 shadow maps + depth map + scene render * max particle limit of 5000).
	if (fireToggle && blurFireParticles)
	{
		// Render each particle using the fire geometry shader.
		for (int i = 0; i < fireParticleCount; i++)
		{
			worldMatrix = renderer->getWorldMatrix();
			worldMatrix *= XMMatrixTranslation(fireParticle[i].position.x, fireParticle[i].position.y, fireParticle[i].position.z);
			pointMesh->sendData(renderer->getDeviceContext());
			fireShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, cameraViewMatrix, cameraProjectionMatrix, NULL, camera, elapsedTime + i, fireParticle[i].size, fireParticle[i].position.y, maxHeight, minHeight, fireBottomColour, fireTopColour, renderNormals);
			fireShader->render(renderer->getDeviceContext(), pointMesh->getIndexCount());
		}
	}

	// Set back buffer as render target and reset view port.
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
}

void App1::depthRender(XMMATRIX world, XMMATRIX view, XMMATRIX projection)
{
	// Use basic depth shader where possible to improve performance as lighting is not calculated. Objects that are affected by vertex manipulation use their own shader.

	// Render water.
	world = renderer->getWorldMatrix();
	world *= XMMatrixTranslation(waterPosition.x, waterPosition.y, waterPosition.z);
	waterMesh->sendData(renderer->getDeviceContext());
	waterShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection, tessProperties, elapsedTime, waterAmplitude, waterFrequency, waterSpeed, camera->getPosition(), viewMatrices, projMatrices, textureMgr->getTexture(L"water_height"));
	waterShader->render(renderer->getDeviceContext(), waterMesh->getIndexCount());
	
	// Render ground.
	world = renderer->getWorldMatrix();
	world *= XMMatrixTranslation(groundPosition.x, groundPosition.y, groundPosition.z);
	groundMesh->sendData(renderer->getDeviceContext());
	terrainShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection, textureMgr->getTexture(L"height"), terrainHeight, viewMatrices, projMatrices, camera->getPosition());
	terrainShader->render(renderer->getDeviceContext(), groundMesh->getIndexCount());

	// Render dog.
	world = renderer->getWorldMatrix();
	world *= XMMatrixScaling(corgi.scale.x, corgi.scale.y, corgi.scale.z);
	world *= XMMatrixRotationY(corgi.rotationY); 
	world *= XMMatrixTranslation(corgi.position.x, corgi.position.y, corgi.position.z);
	world *= XMMatrixRotationY(corgiRotation); 
	world *= XMMatrixTranslation(campfire.position.x, campfire.position.y, campfire.position.z);
	corgiMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection);
	depthShader->render(renderer->getDeviceContext(), corgiMesh->getIndexCount());

	// Render campfire.
	world = renderer->getWorldMatrix();
	world *= XMMatrixScaling(campfire.scale.x, campfire.scale.y, campfire.scale.z);
	world *= XMMatrixRotationY(campfire.rotationY);
	world *= XMMatrixTranslation(campfire.position.x, campfire.position.y, campfire.position.z);
	campfireMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection);
	depthShader->render(renderer->getDeviceContext(), campfireMesh->getIndexCount());

	// Render house.
	world = renderer->getWorldMatrix();
	world *= XMMatrixScaling(house.scale.x, house.scale.y, house.scale.z);
	world *= XMMatrixRotationY(house.rotationY);
	world *= XMMatrixTranslation(house.position.x, house.position.y, house.position.z);
	houseMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection);
	depthShader->render(renderer->getDeviceContext(), houseMesh->getIndexCount());

	// Render lamp.
	world = renderer->getWorldMatrix();
	world *= XMMatrixScaling(lamp.scale.x, lamp.scale.y, lamp.scale.z);
	world *= XMMatrixRotationY(lamp.rotationY);
	world *= XMMatrixTranslation(lamp.position.x, lamp.position.y, lamp.position.z);
	lampMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection);
	depthShader->render(renderer->getDeviceContext(), lampMesh->getIndexCount());

	// Render pier.
	world = renderer->getWorldMatrix();
	world *= XMMatrixScaling(pier.scale.x, pier.scale.y, pier.scale.z);
	world *= XMMatrixRotationY(pier.rotationY);
	world *= XMMatrixTranslation(pier.position.x, pier.position.y, pier.position.z);
	pierMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection);
	depthShader->render(renderer->getDeviceContext(), pierMesh->getIndexCount());

	// Render spheres.
	for (int i = 0; i < SPHERE_COUNT; i++)
	{
		world = renderer->getWorldMatrix();
		world *= XMMatrixScaling(spheres[i].scale.x, spheres[i].scale.y, spheres[i].scale.z);
		world *= XMMatrixRotationY(spheres[i].rotationY);
		world *= XMMatrixTranslation(spheres[i].position.x, spheres[i].position.y, spheres[i].position.z);
		sphereMesh->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection);
		depthShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());
	}

	// Render cubes.
	for (int i = 0; i < CUBE_COUNT; i++)
	{
		world = renderer->getWorldMatrix();
		world *= XMMatrixScaling(cubes[i].scale.x, cubes[i].scale.y, cubes[i].scale.z);
		world *= XMMatrixRotationY(cubes[i].rotationY);
		world *= XMMatrixTranslation(cubes[i].position.x, cubes[i].position.y, cubes[i].position.z);

		cubeMesh->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, projection);
		depthShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());
	}
}

void App1::blurPass()
{
	// Matrices used for rendering the ortho mesh.
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();

	// Calculate the inverse of the view projection matrix to be passed into the shader. Used for calculating world position.
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
	XMMATRIX viewProjectionInverse = XMMatrixMultiply(viewMatrix, projectionMatrix);
	viewProjectionInverse = XMMatrixInverse(nullptr, viewProjectionInverse);
	
	// Calculate the previous view projection matrix to be passed into the shader.
	XMMATRIX previousViewProjection = XMMatrixMultiply(previousView, previousProj);
	
	// Render the ortho mesh across the full screen.
	renderer->setZBuffer(false);
	blurOrthoMesh->sendData(renderer->getDeviceContext());
	motionBlurShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, depthMap->getDepthMapSRV(), sceneRenderTexture->getShaderResourceView(), viewProjectionInverse, previousViewProjection, blurSamples, blurStrength);
	motionBlurShader->render(renderer->getDeviceContext(), blurOrthoMesh->getIndexCount());
	renderer->setZBuffer(true);

	// Set the previous view and projection matrices after applying the motion blur.
	previousView = viewMatrix;
	previousProj = projectionMatrix;
}


void App1::scenePass()
{
	// This is the main pass for rendering the scene.
	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
	XMFLOAT3 position;
	XMFLOAT3 direction;
	XMFLOAT4 specular;

	// Update lights in case their attributes have been modified in ImGui.
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		ambient = lightProperties[i].ambientColour;
		diffuse = lightProperties[i].diffuseColour;
		position = lightProperties[i].position;
		direction = lightProperties[i].direction;
		specular = lightProperties[i].specularColour;

		lights[i]->setAmbientColour(ambient.x, ambient.y, ambient.z, ambient.w);
		lights[i]->setDiffuseColour(diffuse.x, diffuse.y, diffuse.z, diffuse.w);
		lights[i]->setPosition(position.x, position.y, position.z);
		lights[i]->setDirection(direction.x, direction.y, direction.z);
		lights[i]->setSpecularColour(specular.x, specular.y, specular.z, specular.w);
	}

	// Generate the view matrix based on the camera's position.
	camera->update();

	// Get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();

	// Basic skybox - one texture wrapped around sphere. Could be improved using cube mapping to wrap multiple textures.
	// *** //
	// Cull front face.
	renderer->getDeviceContext()->RSSetState(RSCullFront);

	// Disable depth buffer.
	renderer->setZBuffer(false);

	// Render sphere at camera's position so camera is inside the sphere.
	worldMatrix = renderer->getWorldMatrix();
	worldMatrix *= XMMatrixTranslation(camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
	sphereMesh->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"sky"), XMFLOAT4(0, 0, 0, 1));
	textureShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());

	// Re-enable depth buffer.
	renderer->setZBuffer(true);

	// Return to default rasterizer state and set wireframe mode.
	renderer->getDeviceContext()->RSSetState(RSDefault);
	renderer->setWireframeMode(wireframeToggle);
	// *** //

	// Render water.
	// Apply position matrix transformation.
	worldMatrix = renderer->getWorldMatrix();
	worldMatrix *= XMMatrixTranslation(waterPosition.x, waterPosition.y, waterPosition.z);

	// Set both water and light shaders when rendering. The water shader uses light's pixel shader when rendering.
	waterMesh->sendData(renderer->getDeviceContext());
	waterShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, tessProperties, elapsedTime, waterAmplitude, waterFrequency, waterSpeed, camera->getPosition(), viewMatrices, projMatrices, textureMgr->getTexture(L"water_height"));
	lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"water"), lights, camera->getPosition(), lightProperties, specularValues.water, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, true, textureMgr->getTexture(L"water_height"), waterAmplitude, waterResolution); 
	waterShader->render(renderer->getDeviceContext(), waterMesh->getIndexCount());
	
	// Render ground.
	// Apply position matrix transformation.
	worldMatrix = renderer->getWorldMatrix();
	worldMatrix *= XMMatrixTranslation(groundPosition.x, groundPosition.y, groundPosition.z);

	// Set both terrain and light shaders when rendering. The terrain shader uses light's pixel shader when rendering.
	groundMesh->sendData(renderer->getDeviceContext());
	terrainShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"height"), terrainHeight, viewMatrices, projMatrices, camera->getPosition());
	lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"grass"), lights, camera->getPosition(), lightProperties, specularValues.ground, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, true, textureMgr->getTexture(L"height"), terrainHeight, groundResolution);
	terrainShader->render(renderer->getDeviceContext(), groundMesh->getIndexCount());

	// Render corgi.
	// Apply matrix transformations. The corgi has extra transformations for moving around the campfire.
	worldMatrix = renderer->getWorldMatrix();
	worldMatrix *= XMMatrixScaling(corgi.scale.x, corgi.scale.y, corgi.scale.z);
	worldMatrix *= XMMatrixRotationY(corgi.rotationY); // Orient dog.
	worldMatrix *= XMMatrixTranslation(corgi.position.x, corgi.position.y, corgi.position.z); // Corgi's position offset from the campfire which it circles.
	worldMatrix *= XMMatrixRotationY(corgiRotation); // Rotate corgi round campfire.
	worldMatrix *= XMMatrixTranslation(campfire.position.x, campfire.position.y, campfire.position.z); // Move to campfire's position.

	// Render the corgi using the light shader.
	corgiMesh->sendData(renderer->getDeviceContext());
	lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"corgi"), lights, camera->getPosition(), lightProperties, specularValues.dog, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, false, NULL, NULL, NULL);
	lightShader->render(renderer->getDeviceContext(), corgiMesh->getIndexCount());

	// Render campfire.
	// Apply matrix transformations.
	worldMatrix = renderer->getWorldMatrix();
	worldMatrix *= XMMatrixScaling(campfire.scale.x, campfire.scale.y, campfire.scale.z);
	worldMatrix *= XMMatrixRotationY(campfire.rotationY);
	worldMatrix *= XMMatrixTranslation(campfire.position.x, campfire.position.y, campfire.position.z);

	// Render campfire using light shader.
	campfireMesh->sendData(renderer->getDeviceContext());
	lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"campfire"), lights, camera->getPosition(), lightProperties, specularValues.wood, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, false, NULL, NULL, NULL);
	lightShader->render(renderer->getDeviceContext(), campfireMesh->getIndexCount());

	// Render house.
	// Apply matrix transformations.
	worldMatrix = renderer->getWorldMatrix();
	worldMatrix *= XMMatrixScaling(house.scale.x, house.scale.y, house.scale.z);
	worldMatrix *= XMMatrixRotationY(house.rotationY);
	worldMatrix *= XMMatrixTranslation(house.position.x, house.position.y, house.position.z);

	// Render house using light shader.
	houseMesh->sendData(renderer->getDeviceContext());
	lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"house"), lights, camera->getPosition(), lightProperties, specularValues.wood, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, false, NULL, NULL, NULL);
	lightShader->render(renderer->getDeviceContext(), houseMesh->getIndexCount());

	// Render lamp.
	// Apply matrix transformations.
	worldMatrix = renderer->getWorldMatrix();
	worldMatrix *= XMMatrixScaling(lamp.scale.x, lamp.scale.y, lamp.scale.z);
	worldMatrix *= XMMatrixRotationY(lamp.rotationY);
	worldMatrix *= XMMatrixTranslation(lamp.position.x, lamp.position.y, lamp.position.z);

	// Render lamp using light shader.
	lampMesh->sendData(renderer->getDeviceContext());
	lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"metal"), lights, camera->getPosition(), lightProperties, specularValues.metal, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, false, NULL, NULL, NULL);
	lightShader->render(renderer->getDeviceContext(), lampMesh->getIndexCount());

	// Render pier.
	// Apply matrix transformations.
	worldMatrix = renderer->getWorldMatrix();
	worldMatrix *= XMMatrixScaling(pier.scale.x, pier.scale.y, pier.scale.z);
	worldMatrix *= XMMatrixRotationY(pier.rotationY);
	worldMatrix *= XMMatrixTranslation(pier.position.x, pier.position.y, pier.position.z);

	// Render pier using light shader.
	pierMesh->sendData(renderer->getDeviceContext());
	lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"wood"), lights, camera->getPosition(), lightProperties, specularValues.wood, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, false, NULL, NULL, NULL);
	lightShader->render(renderer->getDeviceContext(), pierMesh->getIndexCount());
	
	// Render spheres.
	for (int i = 0; i < SPHERE_COUNT; i++)
	{
		// Apply matrix transformations.
		worldMatrix = renderer->getWorldMatrix();
		worldMatrix *= XMMatrixScaling(spheres[i].scale.x, spheres[i].scale.y, spheres[i].scale.z);
		worldMatrix *= XMMatrixRotationY(spheres[i].rotationY);
		worldMatrix *= XMMatrixTranslation(spheres[i].position.x, spheres[i].position.y, spheres[i].position.z);

		// Render sphere using light shader.
		sphereMesh->sendData(renderer->getDeviceContext());
		lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"metal"), lights, camera->getPosition(), lightProperties, specularValues.metal, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, false, NULL, NULL, NULL);
		lightShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());
	}
	
	for (int i = 0; i < CUBE_COUNT; i++)
	{
		// Apply matrix transformations.
		worldMatrix = renderer->getWorldMatrix();
		worldMatrix *= XMMatrixScaling(cubes[i].scale.x, cubes[i].scale.y, cubes[i].scale.z);
		worldMatrix *= XMMatrixRotationY(cubes[i].rotationY);
		worldMatrix *= XMMatrixTranslation(cubes[i].position.x, cubes[i].position.y, cubes[i].position.z);

		// Render cube using light shader.
		cubeMesh->sendData(renderer->getDeviceContext());
		lightShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"metal"), lights, camera->getPosition(), lightProperties, specularValues.metal, shadowMaps, shadowMapBias, viewMatrices, projMatrices, renderNormals, false, NULL, NULL, NULL);
		lightShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());
	}
	
	// If the fire is enabled.
	if (fireToggle)
	{
		// Render each fire particle.
		for (int i = 0; i < fireParticleCount; i++)
		{
			// Generate fire particle using the fire's position and particle size.
			worldMatrix = renderer->getWorldMatrix();
			worldMatrix *= XMMatrixTranslation(fireParticle[i].position.x, fireParticle[i].position.y, fireParticle[i].position.z);
			pointMesh->sendData(renderer->getDeviceContext());
			fireShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, NULL, camera, elapsedTime + i, fireParticle[i].size, fireParticle[i].position.y, maxHeight, minHeight, fireBottomColour, fireTopColour, renderNormals);
			fireShader->render(renderer->getDeviceContext(), pointMesh->getIndexCount());
		}
	}
	
	// If rendering the light's position is enabled.
	if (renderLights)
	{
		// Iterate through each light.
		for (int i = 0; i < LIGHT_COUNT; i++)
		{
			// If the light is turned on.
			if (lightProperties[i].toggle == true)
			{
				// If the light is a point or spotlight, it will have a position and will be rendered.
				if (lightProperties[i].type == LightMode::POINT || lightProperties[i].type == LightMode::SPOTLIGHT)
				{
					// Move sphere to light's position.
					worldMatrix = renderer->getWorldMatrix();
					worldMatrix *= XMMatrixScaling(0.2, 0.2, 0.2);
					worldMatrix *= XMMatrixTranslation(lights[i]->getPosition().x, lights[i]->getPosition().y, lights[i]->getPosition().z);

					// Render using the texture shader as the sphere doesn't need to be affected by lighting.
					sphereMesh->sendData(renderer->getDeviceContext());
					textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, NULL, lights[i]->getDiffuseColour());
					textureShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());
				}
			}
		}
	}

}

bool App1::render()
{
	// Update corgi's rotated position around the fire.
	corgiRotation += timer->getTime();

	// Increase elapsed time.
	elapsedTime += timer->getTime();

	// Update the fire.
	updateFire(timer->getTime());

	// Depth pass for shadowmaps and depth map.
	depthPass();
	
	// Clear the scene. (default blue colour)
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);

	// If motion blur is enabled.
	if (blurToggle)
	{
		// Render the scene to a texture.
		sceneRenderTexture->setRenderTarget(renderer->getDeviceContext());
		sceneRenderTexture->clearRenderTarget(renderer->getDeviceContext(), 0.0f, 0.0f, 0.0f, 1.0f);
		scenePass();
		renderer->setBackBufferRenderTarget();

		// Blur the scene texture.
		blurPass();
	}
	else
	{
		// Render the scene without rendering to a texture so that wireframe mode works when motion blur is disabled.
		scenePass();
	}
	
	// If the option for rendering shadow maps is enabled...
	if (renderShadowMap)
	{
		// Render shadow map using the shadow map ortho mesh.
		XMMATRIX worldMatrix = renderer->getWorldMatrix();
		XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
		XMMATRIX orthoMatrix = renderer->getOrthoMatrix();

		// Disable depth buffer.
		renderer->setZBuffer(false);
		
		// Render using the texture shader.
		shadowMapMesh->sendData(renderer->getDeviceContext());
		textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, shadowMaps[lightShadowMapToRender - 1][shadowMapToRender - 1]->getDepthMapSRV(), XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
		textureShader->render(renderer->getDeviceContext(), shadowMapMesh->getIndexCount());

		// Enable depth buffer.
		renderer->setZBuffer(true);
	}

	// Render GUI
	gui();

	// Present the rendered scene to the screen.
	renderer->endScene();
	return true;
}

void App1::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Build UI
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Checkbox("Wireframe mode (post-processing must be disabled to use this mode)", &wireframeToggle);
	

	// ImGui options for vertex manipulation.
	if (ImGui::CollapsingHeader("Vertex Manipulation - Water"))
	{
		ImGui::Indent();

		// Sub-section for tessellation.
		// The options are:
		// Reset to default values.
		// Change tessellation mode.
		// Adjust tessellation distance modifiers.
		// Adjust tessellation factors using sliders.
		if (ImGui::CollapsingHeader("Tessellation"))
		{
			ImGui::Indent();

			if (ImGui::Button(string("Reset Tessellation").c_str()))
			{
				tessProperties = defaultTessProperties;
			}

			const char* mode_names[TessellationMode::OFF + 1] = { "DISTANCE", "SLIDERS", "OFF" };
			static int current_selection = TessellationMode::DISTANCE;
			const char* current_selection_name = (current_selection >= 0 && current_selection < TessellationMode::OFF + 1) ? mode_names[current_selection] : "Unknown";

			ImGui::SliderInt("Tessellation Mode", &current_selection, 0, TessellationMode::OFF, current_selection_name);

			tessProperties.mode = TessellationMode(current_selection);

			if (tessProperties.mode == TessellationMode::DISTANCE)
			{
				ImGui::SliderFloat("Maximum Distance", &tessProperties.maxDistance, 0, 100);
				ImGui::SliderFloat("Minimum Distance", &tessProperties.minDistance, 0, 100);
				ImGui::SliderFloat("Maximum Tessellation Factor", &tessProperties.maxFactor, 1, 64);
				ImGui::SliderFloat("Minimum Tessellation Factor", &tessProperties.minFactor, 1, 64);
			}

			if (tessProperties.mode == TessellationMode::SLIDERS)
			{
				ImGui::SliderFloat4("Edge Tessellation Factor", &tessProperties.edgeFactor.x, 1, 64);
				ImGui::SliderFloat2("Inside Tessellation Factor", &tessProperties.insideFactor.x, 1, 64);
			}

			ImGui::Unindent();
		}

		// Sub-section for wave properties.
		// Can adjust the wave's height and speed.
		if (ImGui::CollapsingHeader("Wave Properties"))
		{
			ImGui::Indent();
			ImGui::SliderFloat("Water Wave Amplitude", &waterAmplitude, 0, 1);
			ImGui::SliderFloat("Water Wave Speed", &waterSpeed, 0, 1);
			ImGui::Unindent();
		};
		ImGui::Unindent();
	}

	// Options for lighting.
	if (ImGui::CollapsingHeader("Lights"))
	{
		ImGui::Indent();

		// Toggle rendering normals and light position.
		ImGui::Checkbox(string("Toggle Normal Rendering").c_str(), &renderNormals);
		ImGui::Checkbox(string("Toggle Light Position Rendering").c_str(), &renderLights);

		// Adjust specular power for each material.
		if (ImGui::CollapsingHeader("Specular Material Values (100 = OFF)"))
		{
			if (ImGui::Button("Reset Specular Values"))
			{
				specularValues = defaultSpecularValues;
			}
			ImGui::SliderFloat("Water Specular", &specularValues.water, 1, 100);
			ImGui::SliderFloat("Ground Specular", &specularValues.ground, 1, 100);
			ImGui::SliderFloat("Dog Specular", &specularValues.dog, 1, 100);
			ImGui::SliderFloat("Metal Specular", &specularValues.metal, 1, 100);
			ImGui::SliderFloat("Wood Specular", &specularValues.wood, 1, 100);
		}
		
		ImGui::Unindent();

		const char* mode_names[LightMode::SPOTLIGHT + 1] = { "DIRECTIONAL", "POINT", "SPOTLIGHT" };

		ImGui::Indent();

		// Each light has the following options:
		// Toggle on/off
		// Reset properties
		// Change mode
		// Change properties such as colour, direction, attenuation, and spotlight cutoffs based on the mode
		for (int i = 0; i < LIGHT_COUNT; i++)
		{
			int lightNo = i + 1;
			std::string lightString = std::to_string(lightNo);
			if (ImGui::CollapsingHeader(string("Light " + lightString).c_str()))
			{
				ImGui::Checkbox(string(lightString + " - On/Off").c_str(), &lightProperties[i].toggle);
				if (lightProperties[i].toggle)
				{
					const char* current_selection_name = (lightProperties[i].type >= 0 && lightProperties[i].type < LightMode::SPOTLIGHT + 1) ? mode_names[lightProperties[i].type] : "Unknown";

					if (ImGui::Button(string(lightString + " - Reset").c_str()))
					{
						lightProperties[i] = defaultLightProperties[i];
					}

					ImGui::SliderInt(string(lightString + " - Light Mode").c_str(), &lightProperties[i].type, 0, LightMode::SPOTLIGHT, current_selection_name);

					if (lightProperties[i].type == LightMode::DIRECTIONAL)
					{
						ImGui::SliderFloat3(string(lightString + " - Position").c_str(), &lightProperties[i].position.x, -50, 50);
						ImGui::SliderFloat3(string(lightString + " - Direction").c_str(), &lightProperties[i].direction.x, -1, 1);
						ImGui::ColorEdit3(string(lightString + " - Diffuse Colour").c_str(), &lightProperties[i].diffuseColour.x);
						ImGui::ColorEdit3(string(lightString + " - Ambient Colour").c_str(), &lightProperties[i].ambientColour.x);
						ImGui::ColorEdit3(string(lightString + " - Specular Colour").c_str(), &lightProperties[i].specularColour.x);
					}
					else if (lightProperties[i].type == LightMode::POINT)
					{
						ImGui::SliderFloat3(string(lightString + " - Position").c_str(), &lightProperties[i].position.x, -50, 50);
						ImGui::SliderFloat3(string(lightString + " - Attenuation").c_str(), &lightProperties[i].attenuation.x, 0, 1);
						ImGui::ColorEdit3(string(lightString + " - Diffuse Colour").c_str(), &lightProperties[i].diffuseColour.x);
						ImGui::ColorEdit3(string(lightString + " - Ambient Colour").c_str(), &lightProperties[i].ambientColour.x);
						ImGui::ColorEdit3(string(lightString + " - Specular Colour").c_str(), &lightProperties[i].specularColour.x);
					}
					else if (lightProperties[i].type == LightMode::SPOTLIGHT)
					{
						ImGui::SliderFloat3(string(lightString + " - Position").c_str(), &lightProperties[i].position.x, -50, 50);
						ImGui::SliderFloat3(string(lightString + " - Attenuation").c_str(), &lightProperties[i].attenuation.x, 0, 1);
						ImGui::SliderFloat3(string(lightString + " - Direction").c_str(), &lightProperties[i].direction.x, -1, 1);
						ImGui::SliderFloat(string(lightString + " - Inner Spotlight Cutoff").c_str(), &lightProperties[i].innerSpotlightCutoff, 0, 1);
						if (lightProperties[i].outerSpotlightCutoff > lightProperties[i].innerSpotlightCutoff)
						{
							lightProperties[i].outerSpotlightCutoff = lightProperties[i].innerSpotlightCutoff;
						}
						ImGui::SliderFloat(string(lightString + " - Outer Spotlight Cutoff").c_str(), &lightProperties[i].outerSpotlightCutoff, 0, lightProperties[i].innerSpotlightCutoff);
						ImGui::SliderFloat(string(lightString + " - Spotlight Falloff Factor").c_str(), &lightProperties[i].spotlightFalloff, 0, 3);
						ImGui::ColorEdit3(string(lightString + " - Diffuse Colour").c_str(), &lightProperties[i].diffuseColour.x);
						ImGui::ColorEdit3(string(lightString + " - Ambient Colour").c_str(), &lightProperties[i].ambientColour.x);
						ImGui::ColorEdit3(string(lightString + " - Specular Colour").c_str(), &lightProperties[i].specularColour.x);
					}
				}
			}
		}
		
		ImGui::Unindent();
	}

	// Options for shadows:
	// Toggle rendering shadow map in corner
	// Select which shadow map to render
	// Adjust shadow map scene size for directional lights
	// Adjust shadow map bias
	// Adjust near and far cutoff values
	if (ImGui::CollapsingHeader("Shadows"))
	{
		ImGui::Indent();

		ImGui::Checkbox("Toggle Shadow Map Display", &renderShadowMap);
		if (renderShadowMap)
		{
			ImGui::SliderInt("Light Shadow Map to Display", &lightShadowMapToRender, 1, LIGHT_COUNT);
			if (lightProperties[lightShadowMapToRender - 1].type == LightMode::POINT)
			{
				ImGui::SliderInt("Side to Display", &shadowMapToRender, 1, 6);
			}
			else
			{
				shadowMapToRender = 1;
			}
		}
		ImGui::SliderInt("Shadow Map Scene Size", &sceneSize, 25, 200);
		ImGui::SliderFloat("Shadow Map Bias", &shadowMapBias, 0.0f, 0.2f);
		ImGui::SliderFloat("Directional Lights Near Cutoff", &directionalNear, 0.01f, 10.0f);
		ImGui::SliderFloat("Directional Lights Far Cutoff", &directionalFar, 20.0f, 200.0f);
		ImGui::SliderFloat("Spot/Point Lights Near Cutoff", &spotPointNear, 0.01f, 10.0f);
		ImGui::SliderFloat("Spot/Point Lights Far Cutoff", &spotPointFar, 20.0f, 200.0f);
		
		ImGui::Unindent();
	}

	// Fire geometry shader options:
	// Toggle on/off
	// Restart the fire - used if the fire breaks
	// Adjust number of particles
	// Adjust particle size and speed
	// Adjust fire height and width
	// Adjust fire colours
	if (ImGui::CollapsingHeader("Geometry Generation - Fire"))
	{
		ImGui::Indent();

		ImGui::Checkbox("Fire On/Off", &fireToggle);
		
		if (fireToggle)
		{
			if (ImGui::Button("Restart Fire"))
			{
				resetFire();
			}


			ImGui::Text("Fire position is locked to Light 2's position.");

			/*
			XMFLOAT3 firePos = firePosition;
			ImGui::SliderFloat3("Fire Position", &firePosition.x, -50, 50);
			if (firePos.x != firePosition.x || firePos.y != firePosition.y || firePos.z != firePosition.z)
			{
				resetFire();
			}
			*/

			int particleCount = fireParticleCount;
			ImGui::SliderInt("Number of Particles", &fireParticleCount, 0, 5000);
			if (particleCount != fireParticleCount)
			{
				resetFire();
			}

			ImGui::SliderFloat("Particle Size", &particleSize, 0, 3);
			ImGui::SliderFloat("Particle Speed", &particleSpeed, 0, 1.5);

			float height = fireHeight;
			ImGui::SliderFloat("Fire Height", &fireHeight, 0, 6);
			if (fireHeight != height)
			{
				resetFire();
			}

			ImGui::SliderFloat("Fire Width", &fireWidth, 0, 3);

			ImGui::ColorEdit4("Top Colour", &fireTopColour.x);
			ImGui::ColorEdit4("Bottom Colour", &fireBottomColour.x);
		}

		ImGui::Unindent();
	}

	// Motion blur options:
	// Toggle on/off
	// Adjust samples and strength
	// Toggle blurring fire particles
	if (ImGui::CollapsingHeader("Post Processing - Motion Blur"))
	{
		ImGui::Indent();

		ImGui::Checkbox("Motion Blur On/Off", &blurToggle);
		ImGui::SliderInt("Blur Samples", &blurSamples, 1, 10);
		ImGui::SliderFloat("Blur Strength", &blurStrength, 1, 5);
		ImGui::Checkbox("Blur Fire Particles", &blurFireParticles);

		ImGui::Unindent();
	}
	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

