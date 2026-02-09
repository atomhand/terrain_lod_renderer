#include "gpu_render.h"

void Engine::CullingFilter::Cull(GpuRender& gpuRender, glm::mat4& vp, RenderPassId pass, StorageBuffer& input, StorageBuffer& output, StorageBuffer& inputCount, StorageBuffer& outputCount) {
    gpuRender.materialHeadersBuffer.BindBase(6);
    gpuRender.renderItemBuffer.BindBase(7);
    filter.Filter(input,output,inputCount,outputCount);
}