#pragma once
#include <glm/glm.hpp>
#include "entt/entt.hpp"
#include "environment_map.h"

namespace Engine {
    struct Transform {
    public:
        glm::vec3 position() {
            return glm::vec3(global * glm::vec4(0.f,0.f,0.f,1.f));
        }
        glm::mat4 global;
    };

    struct  Input {
    public:
        glm::vec2 mousePos;
        glm::vec2 mousePosDelta;
        float scrollDelta;

        glm::vec2 keyAxisDelta;

        float animSpeed = 3.0;
        
        float deltaTime;
        bool vsync = true;

        bool testFlythrough = false;

        // Windows
        bool terrainGeometryDebug = false;
        bool terrainCalibrationWindow = false;
        bool profilerWindow = true;
        bool radixSortTester = false;
        bool gpuFilterTester = false;

        // Camera control
        bool flyCamera = true;
        bool noClip = true;

        // Debug render
        bool enableRendering = true;
        bool enableGpuProfiling = true;
        bool debugMetaCam = false;
        bool wireFrame = false;
        bool previewTriangleDensity = false;
        bool drawAABBs = false;
        int previewNormalsMode = 0;
        bool previewChunksMode = false;

        // Main render config
        bool enableCulling = true;
        bool applyAutoExposure = true;

        // debug lights
        int shadowTestingMode = 0;
        bool previewCascades = false;
        int numCascades = 5;
        float lightPssmFactor = 0.f;

        // Terrain render     
        bool stochasticBlending = true;
        float displacementScale = 2.0;
        int lodControlParam = 8; // roughly ~= pixel length of terrain mesh edges
        bool computeTerrain = true;
        bool lodMorphs = true;
        bool forceFullGeneration = false;

        // fog
        bool drawFog = true;
        float fogStrength = 0.15f;
        float constantFogFactor = 0.0f;
        float heightFogFactor = 1.0f;
        float heightFogTransitionStart = 256.f;
        float heightFogTransitionDuration = 256.f;


        bool drawShadows() { return shadowTestingMode != 2; }

        int viewDistanceParam = 0;
    };

    class World {
    public:
        Input input;
        float shaderAnimTime;
        entt::registry registry;
        Engine::EnvironmentMap skybox = Engine::EnvironmentMap("skybox/farm_field_puresky_4k.hdr");

        template <typename T>
        T& GetSingle() {            
            auto view = registry.view<T>();
            return view.get<T>(view.front());
        }

        // 
        float animDeltaTime() {
            return input.deltaTime * input.animSpeed * 0.2f;
        }
    };
}