// Tom Kellett 2025
#pragma once

#include <unordered_map>

#include "world.h"
#include "shader_shared.h"
#include "storage_buffer.h"
#include "shader.h"

namespace Engine {
    enum class RenderPassId : uint32_t {
        OPAQUE,
        POST_OPAQUE,
        TRANSPARENT,
        SHADOW,
        DIAGNOSTIC
    };

    // Same struct used on CPU and GPU side
    struct MaterialHeader {
        uint32_t id;
        uint32_t drawBufferOffset;
        uint32_t drawCount;
        uint32_t drawKeyOffset; // not set on  CPU side

        uint32_t renderPassesMask;        
        uint32_t shadowMaterialRedirect = 0xffffffff;

        void SetRenderPass(RenderPassId pass, bool x = true) {
            uint32_t position = static_cast<uint32_t>(pass);
            // clear bit and then set it
            renderPassesMask = (renderPassesMask & ~((uint32_t)1 << position) | (uint32_t(x) << position));
        }

        bool IsRenderPassEnabled(RenderPassId pass) {
            uint32_t position = static_cast<uint32_t>(pass);
            return (renderPassesMask >> position) & uint32_t(1);
        }

        bool operator < (MaterialHeader const& rhs) {
            return this->id < rhs.id;
        }
    };

    struct GpuMaterialInstance {
        uint32_t materialId;
        uint32_t meshId;
        // id within the material's internal buffer, used to access material specific per-instance attributes
        // Can be uninitialized if the material doesn't care
        uint32_t materialInstanceId;
    };

    class MaterialImplementation {
    public:
        std::string name;
        std::unordered_map<RenderPassId,Shader> passShaders;

        // called once per frame, opportunity to fill buffers
        virtual void Prepare(World& world) {};

        virtual void Bind(World& world) {}

        virtual void Render(World& world, uint32_t offset, uint32_t count, RenderPassId pass, std::unordered_map<RenderPassId,Shader>& defaultShaders) {
            if(passShaders.contains(pass)) {
                passShaders[pass].use();
            } else if(defaultShaders.contains(pass)) {
                defaultShaders[pass].use();
            } else {
                std::cout << "Trying to run material " << name << " for pass " << int(pass) << " but no shader is available";
                return;
            }

            Bind(world);
            glMultiDrawElementsIndirect(GL_TRIANGLES,  GL_UNSIGNED_INT, (void*)(offset*sizeof(DrawElementsIndirectCommand)), count, 0);
        }

        MaterialImplementation(const char* name) : name(name) {}

        void AddShaderForPass(Engine::Shader& shader, RenderPassId pass) {
            passShaders[pass] = shader;
        }
    };

    template <typename InstanceData> class InstancedMaterialImplementation : public MaterialImplementation {
    protected:
        std::vector<InstanceData> instancesData;
        StorageBuffer instancesDataBuffer = StorageBuffer(0);

    public:
        void Prepare(World& world) override {
            auto instancesDataView = world.registry.view<InstanceData,GpuMaterialInstance>();
            
            instancesData.clear();

            for(auto entity : instancesDataView) {
                auto  [instanceData,materialInstance] = instancesDataView.get(entity);
                materialInstance.materialInstanceId = instancesData.size();

                instancesData.push_back(instanceData);
            }

            instancesDataBuffer.Set<InstanceData>(instancesData.data(), instancesData.size(), 0, true);
        }

        void Bind(World& world) override {
            instancesDataBuffer.BindBase(6);
        }

        InstancedMaterialImplementation(const char* name) : MaterialImplementation(name) {};
    };

    struct MaterialRenderComponent {
    public:
        std::unique_ptr<MaterialImplementation> renderPass;

        MaterialRenderComponent(MaterialImplementation* renderPass) : renderPass(renderPass) {};
    };


    struct DirectDrawMaterial {
    protected:
        int modelLocation = -2;
        int normalMatrixLocation = -2;
        int colorLocation = -2;
    public:
        Shader shader;

        virtual void use() {
            shader.use();
        }        

        // Make sure to bind the shader first
        void SetModel(glm::mat4 &model) {
            if(modelLocation == -2) {
                modelLocation = glGetUniformLocation(shader.programId(), "model");
            }
            glUniformMatrix4fv(modelLocation,1, GL_FALSE, &model[0][0]);
        }

        // Make sure to bind the shader first
        void SetColor(glm::vec3 color) {
            if(colorLocation == -2) {
                colorLocation = glGetUniformLocation(shader.programId(), "color");
            }
            glUniform3fv(colorLocation, 1, &color[0]);
        }

        DirectDrawMaterial(Shader shader) : shader(shader) {};
    };
}