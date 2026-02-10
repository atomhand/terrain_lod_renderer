#include "gpu_render.h"

void Engine::CullingFilter::Cull(GpuRender& gpuRender, RenderPassId pass, StorageBuffer& input, StorageBuffer& output, StorageBuffer& inputCount, StorageBuffer& outputCount) {
    gpuRender.materialHeadersBuffer.BindBase(6);
    gpuRender.renderItemBuffer.BindBase(7);
    if(pass == RenderPassId::SHADOW) {
        shadowFilter.Filter(input,output,inputCount,outputCount);
    } else {
        filter.Filter(input,output,inputCount,outputCount);
    }
}