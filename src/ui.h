#pragma once

#include "world.h"
#include "imgui.h"

static void DrawConfigWindow(bool* p_open, Engine::World& world) {
    if(!ImGui::Begin("Configuration", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    
    ImGui::SeparatorText("Windows");
    ImGui::Checkbox("Terrain Geometry debug", &world.input.terrainGeometryDebug);
    ImGui::Checkbox("Terrain Generation Calibration", &world.input.terrainCalibrationWindow);
    ImGui::Checkbox("Profiler", &world.input.profilerWindow);
    ImGui::Checkbox("Radix sort test", &world.input.radixSortTester);
    ImGui::Checkbox("Gpu filter text", &world.input.gpuFilterTester);

    ImGui::SeparatorText("Debug");

    ImGui::Checkbox("enableRendering", &world.input.enableRendering);
    ImGui::Checkbox("debugMetaCam", &world.input.debugMetaCam);
    ImGui::Checkbox("drawAABBs", &world.input.drawAABBs);
    ImGui::Checkbox("wireFrame", &world.input.wireFrame);
    ImGui::Checkbox("previewTriangleDensity", &world.input.previewTriangleDensity);
    ImGui::Checkbox("gpu profiling", &world.input.enableGpuProfiling);

    
    ImGui::SeparatorText("Render config");

    ImGui::Checkbox("Culling", &world.input.enableCulling);
    ImGui::Checkbox("prepass", &world.input.prepass);
    ImGui::Checkbox("autoexposure", &world.input.applyAutoExposure);
    ImGui::Checkbox("vsync", &world.input.vsync);
    
    ImGui::SeparatorText("Debug Lights");

    ImGui::Checkbox("previewCascades", &world.input.previewCascades);
    ImGui::SliderInt("shadowTestingMode", &world.input.shadowTestingMode, 0, 2);
    ImGui::SliderInt("previewNormalsMode", &world.input.previewNormalsMode, 0, 3);
    ImGui::SliderInt("numCascades", &world.input.numCascades, 1, 16);
    ImGui::SliderFloat("lightPssmFactor", &world.input.lightPssmFactor, 0.0f,1.0f);

    ImGui::SeparatorText("Terrain Rendering");

    ImGui::SliderInt("viewDistanceParam", &world.input.viewDistanceParam, 0, 2);
    ImGui::Checkbox("stochasticBlending", &world.input.stochasticBlending);
    ImGui::SliderFloat("displacement Scale", &world.input.displacementScale, 0.0, 25.0);
    ImGui::SliderInt("LoD control param", &world.input.lodControlParam, 1, 32);
    ImGui::Checkbox("Compute Driven Terrain LoD", &world.input.computeTerrain);
    ImGui::Checkbox("LoD transition morphs", &world.input.lodMorphs);

    
    ImGui::SeparatorText("Fog");
    ImGui::Checkbox("drawFog", &world.input.drawFog);
    ImGui::SliderFloat("fogStrength", &world.input.fogStrength, 0.0, 1.0);
    ImGui::SliderFloat("constantFogFactor", &world.input.constantFogFactor, 0.0, 1.0);
    ImGui::SliderFloat("heightFogFactor", &world.input.heightFogFactor, 0.0, 1.0);
    ImGui::SliderFloat("heightFogTransitionStart", &world.input.heightFogTransitionStart, 0.0f, 2048.f);
    ImGui::SliderFloat("heightFogTransitionDuration", &world.input.heightFogTransitionDuration, 0.0f, 2048.f);

    ImGui::SeparatorText("Movement");

    ImGui::Checkbox("noClip", &world.input.noClip);
    
    ImGui::End();
}

static void DrawUi(Engine::World& world) {
    // Verify ABI compatibility between caller code and compiled version of Dear ImGui. This helps detects some build issues.
    IMGUI_CHECKVERSION();

    bool p_open = !world.input.flyCamera;

    if(p_open)
        DrawConfigWindow(&p_open, world);

    world.input.flyCamera = !p_open;
};