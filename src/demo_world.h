#pragma once
#include "world.h"
#include "rts_camera.h"

// Container for the demo data so it can easily be passed between parts of the application
// As a future extension, the World class will have a dynamic registry (ECS pattern)
// removing the need to manually implement the stored data types in this container
class DemoWorld : public Engine::World {
public:
    RtsCamera camera;
};