#pragma once
#include "world.h"
#include "fastnoise/FastNoise.h"
#include "imgui.h"
#include "shader_shared.h"

class Terrain {
    FastNoise::SmartNode<FastNoise::Add> noise;

    struct TerrainConfig {
        float PERIOD  = 30000.0;

        float FINAL_SCALE  = 4000.0;

        bool erosionEnabled = false;
        float EROSION_PERIOD  = 1000.0;
        float EROSION_SCALE  = 0.25f;
        float EROSION_STRENGTH = 0.1f; // 0.04
        int EROSION_OCTAVES = 4;
        int SEED = 0;

        float foothillsFreq = 1.f;
        float foothillsScale = 0.25f;
        int FOOTHILL_OCTAVES = 8;//8;

        float mountainFreq = 0.4f;
        float mountainScale = 10.f;
        int MOUNTAIN_OCTAVES = 10;//10;
        int mountainExponent = 2;

        bool operator==(const TerrainConfig&) const = default;
    };

    TerrainConfig config;

    float Erosion(glm::vec2 pos, float height, glm::vec3 normal, float a, int octaves, float& maxErosion);

    std::vector<float> heightTemp;

public:
    Engine::TerrainNoiseUniformData GetTerrainNoiseUniform() {
        Engine::TerrainNoiseUniformData uniform;

        uniform.noisePeriod = config.PERIOD;
        uniform.noiseScale = config.FINAL_SCALE;
        uniform.noiseFoothillsFreq = config.foothillsFreq;
        uniform.noiseFoothillsScale = config.foothillsScale;
        uniform.noiseMountainFreq = config.mountainFreq;
        uniform.noiseMountainScale = config.mountainScale;
        uniform.noiseMountainExponent = config.mountainExponent;
        uniform.noiseFoothillOctaves = config.FOOTHILL_OCTAVES;
        uniform.noiseMountainOctaves = config.MOUNTAIN_OCTAVES;

        return uniform;
    }

    bool CalibrationUi(Engine::World& world) {
        if(world.input.terrainCalibrationWindow) {
            TerrainConfig oldConfig = TerrainConfig(config);
            ImGui::Begin("TerrainCalibrationWindow", &world.input.terrainCalibrationWindow);

            ImGui::InputInt("Seed", &config.SEED);
            ImGui::SliderFloat("Noise Period", &config.PERIOD, 5000.f, 100000.f);
            ImGui::SliderFloat("Overall Scale", &config.FINAL_SCALE, 100.f, 8000.f);

            // Erosion
            ImGui::SeparatorText("Erosion");
            ImGui::Checkbox("Erosion enabled", &config.erosionEnabled);
            ImGui::SliderFloat("Erosion Period", &config.EROSION_PERIOD, 100.f, 4000.f);
            ImGui::SliderFloat("Erosion Scale", &config.EROSION_SCALE, 0.f, 10.f);
            ImGui::SliderFloat("Erosion Strength", &config.EROSION_STRENGTH, 0.01f, 1.0f);
            ImGui::SliderInt("Erosion octaves", &config.EROSION_OCTAVES, 1, 10);

            // MOUNTAINS
            ImGui::SeparatorText("Mountains");

            ImGui::SliderInt("Mountain Octaves", &config.MOUNTAIN_OCTAVES, 1, 16);
            ImGui::SliderFloat("Mountain Frequency", &config.mountainFreq, 0.1f, 2.f);
            ImGui::SliderFloat("Mountain Scale", &config.mountainScale, 0.f, 10.f);
            ImGui::SliderInt("Mountain Exponent", &config.mountainExponent, 1, 5);

            // FOOTHILLS
            ImGui::SeparatorText("Foothills");

            ImGui::SliderInt("Foothill Octaves", &config.FOOTHILL_OCTAVES, 1, 16);
            ImGui::SliderFloat("Foothill Frequency", &config.foothillsFreq, 0.1f, 2.f);
            ImGui::SliderFloat("Foothill Scale", &config.foothillsScale, 0.f, 10.f);

            bool configChanged = config != oldConfig;

            ImGui::End();

            if (configChanged) ConfigureNoise();

            return configChanged;
        } else {
            return false;
        }        
    }

    float MaxHeight() {
        return config.mountainScale * config.FINAL_SCALE * 0.5f + 4.f;;
    }

    float Height(float x, float z);
    
    void SampleRegion(glm::vec2 origin, glm::vec2 extent, int cellW, std::span<float> output, std::span<glm::vec2> extraOutput);

    Terrain() {
        ConfigureNoise();
    }

    void ConfigureNoise() {        auto fnMountainsSrc = FastNoise::New<FastNoise::Simplex>();

        // FOOTHILLS

        auto fnFoothillsSrc = FastNoise::New<FastNoise::Simplex>();
        auto fnFoothillsFbm = FastNoise::New<FastNoise::FractalFBm>();

        fnFoothillsSrc ->SetSeedOffset(32112);
        fnFoothillsSrc->SetScale(config.PERIOD / config.foothillsFreq);

        fnFoothillsFbm->SetSource(fnFoothillsSrc);
        fnFoothillsFbm->SetOctaveCount(config.FOOTHILL_OCTAVES);

        // Fbm output is normalised to -2..2 for some reason
        auto fnFoothills = FastNoise::New<FastNoise::Multiply>();
        fnFoothills->SetLHS(fnFoothillsFbm);
        fnFoothills->SetRHS(config.foothillsScale * 0.5f);

        // intermediate
        auto fnLandScale = FastNoise::New<FastNoise::Add>();
        fnLandScale->SetLHS(fnFoothillsFbm);
        fnLandScale->SetRHS(2.0f);

        auto fnLandScale2 = FastNoise::New<FastNoise::Divide>();
        fnLandScale2->SetLHS(fnFoothillsFbm);
        fnLandScale2->SetRHS(3.0f);

        // MOUNTAINS

        auto fnMountainsFbm = FastNoise::New<FastNoise::FractalFBm>();

        fnMountainsSrc ->SetSeedOffset(4432);
        fnMountainsSrc->SetScale(config.PERIOD / config.mountainFreq);

        fnMountainsFbm->SetSource(fnMountainsSrc);
        fnMountainsFbm->SetOctaveCount(config.MOUNTAIN_OCTAVES);

        // Fbm output is normalised to -2..2 for some reason
        auto fnMountainsShift = FastNoise::New<FastNoise::Multiply>();
        fnMountainsShift->SetLHS(fnMountainsFbm);
        fnMountainsShift->SetRHS(0.5f);

        auto fnMountainsScaled = FastNoise::New<FastNoise::Multiply>();
        fnMountainsScaled->SetLHS(fnMountainsShift);
        fnMountainsScaled->SetRHS(fnLandScale2);

        auto fnMountainsPow = FastNoise::New<FastNoise::PowInt>();
        fnMountainsPow->SetValue(fnMountainsScaled);
        fnMountainsPow->SetPow(config.mountainExponent);

        auto fnMountains = FastNoise::New<FastNoise::Multiply>();
        fnMountains->SetLHS(fnMountainsPow);
        fnMountains->SetRHS(config.mountainScale);

        // OUTPUT

        noise = FastNoise::New<FastNoise::Add>();
        noise->SetLHS(fnMountains);
        noise->SetRHS(fnFoothills);
    }
};