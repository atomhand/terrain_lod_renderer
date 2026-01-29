// Tom Kellett 2025
#include <iostream>
#include <utility>
#include "shader.h"

#include "asset_helper.h"
#include "application.h"
#include "assimp_wrapper.h"
#include "renderpasses.h"

#include "world.h"
#include "terrain.h"
#include "terrain_geometry.h"

#include "fly_camera.h"
#include "bird_flock.h"

#include "light.h"

#include "ui.h"
#include "profiler.h"

#include "gpu_sort.h"
#include "gpu_filter.h"

using namespace std;

const char * title = "GPU Programming Coursework App";
Engine::Application app = Engine::Application(1920,1080,title);
Engine::World world;

// edge length of the terrain (counted in chunks/quadtrees)
#ifdef DEBUG
const unsigned int terrainEdgeLength = 3;
#else
const unsigned int terrainEdgeLength = 5;
#endif

// Size of a terrain cell
const float chunkSize = 256000.0;

int main()
{
	Engine::AssetHelper::CreateDirectories();

	RenderPasses renderPasses;

	Engine::GpuSortTester gpuSortTester;
	Engine::GpuFilterTester gpuFilterTester;

	// Terrain
	auto terrain_entity = world.registry.create();
	auto terrainGeometry = TerrainGeometry::Insert(world,terrain_entity,terrainEdgeLength,chunkSize);
	world.registry.emplace<Terrain>(terrain_entity);

	// Camera
#ifdef DEBUG
	FlyCamera::Setup(world, glm::vec3(0.f,128.f,0.f),terrainGeometry.SuggestFarPlane());
#else
	FlyCamera::Setup(world, glm::vec3(0.f,128.f,0.f), terrainGeometry.SuggestFarPlane());
#endif

	// Sun
	auto &sun = world.registry.emplace<Engine::DirectionalLight>(world.registry.create());
	sun.direction= glm::normalize(glm::vec3(2.0,-1.,-4.0));
	sun.color = glm::vec3(15.0,15.,15.);

	BirdFlockManager::Setup(world);

	renderPasses.Init(world);
	// event loop
	while(!app.shouldClose()) {
		app.frameStart(world);

		FlyCamera::Update(world);

		Engine::UpdateCameraSystem(world);
		BirdFlockManager::Update(world);
		TerrainGeometry::Update(world);

		renderPasses.RunAll(world,app);

		world.shaderAnimTime = fmod(world.shaderAnimTime + world.animDeltaTime(), 1000.f);

		DrawUi(world);

		if(world.input.radixSortTester)
			gpuSortTester.DrawInterface(world);
		if(world.input.gpuFilterTester)
			gpuFilterTester.DrawInterface(world);

		Engine::Profiler::Render(world);

		app.frameEnd(world);
	}
	
    return 0;
}