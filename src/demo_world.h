#pragma once
#include "world.h"

#include "rts_camera.h"
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct InstancedRenderBatch
{    
    Engine::Shader shader;
    std::vector<glm::mat4x4> transforms;

    unsigned int num_indices;

    GLuint positionBufferObject;
    GLuint vao;

    InstancedRenderBatch(Engine::Shader shader) : shader(shader) {

    };
};

// Container for the demo data so it can easily be passed between parts of the application
// As a future extension, the World class will have a dynamic registry (ECS pattern)
// removing the need to manually implement the stored data types in this container
class DemoWorld : public World {
public:
    RtsCamera camera;

    
    std::vector<InstancedRenderBatch> instanced_render_batches;
};