#pragma once
#include <chrono>
#include <iostream>
#include <fstream>

#include "world.h"
#include "asset_helper.h"

using Engine::World;

class Tester {
    void Writefile() {
        auto path = Engine::AssetHelper::screenshotPath("profilingResult.csv");

        std::ofstream file(path);

        for(int phase= 0; phase<4; phase++) {
            file << "Frame time (ms) - " << phaseNames[phase] << ",,,,";
        }

        file << std::endl;
        for(int phase=0; phase<4; phase++)
            for(int config =0; config<4; config++) {
                file << configNames[config] << ", ";
            }
        file << std::endl;

        for(int i=0; i<PART_FRAMES; i++) {
            for(int phase=0; phase<4; phase++)
                for(int config =0; config<4; config++) {
                    file << frameTimes[config][phase*PART_FRAMES + i] << ", ";
                }
            file << std::endl;
        }

        file.close();
    }
public:
    static inline const int PART_FRAMES = 450;
    static inline const int MAX_FRAMES = PART_FRAMES*4;

    int currentFrame;
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::vector<float> frameTimes[4];

    int currentEvaluationConfig = 0;

    static inline std::vector<const char*> configNames = {
        "GPU terrain",
        "GPU terrain (geometry only)",
        "CPU terrain",
        "CPU terrain (geometry only)"
    };

    static inline std::vector<const char*> phaseNames = {
        "Slow traversal",
        "Fast traversal",
        "Zoom in",
        "Zoom out"
    };

    static void Update(World& world) {
        using namespace std::literals;

        auto testerView = world.registry.view<Tester>();
        if(world.input.testFlythrough) {
            auto testerEnt = testerView.front();
            if(testerEnt == entt::null) {
                testerEnt = world.registry.create();
                auto& tester = world.registry.emplace<Tester>(testerEnt);

                tester.start = std::chrono::steady_clock::now();

                // set up params for test
                world.input.forceFullGeneration = true;
                world.input.flyCamera = true;                
                world.input.enableGpuProfiling = false;
                world.input.vsync = false;
                world.input.viewDistanceParam = 4;
                world.input.previewTriangleDensity = false;

                world.input.computeTerrain = true;
            } else {
                Tester& tester = testerView.get<Tester>(testerEnt);
                tester.currentFrame += 1;
                tester.frameTimes[tester.currentEvaluationConfig].push_back(world.input.deltaTime * 1000.f);

                if(tester.currentFrame == MAX_FRAMES) {
                    auto end = std::chrono::steady_clock::now();
                    auto timing = (std::chrono::duration_cast<std::chrono::microseconds>(end-tester.start) / 1ms) / MAX_FRAMES;
                    std::cout << "Test " << tester.currentEvaluationConfig << "(" << phaseNames[tester.currentEvaluationConfig] << ") complete, time per frame " << timing << "ms" << std::endl;
                    tester.currentEvaluationConfig += 1;

                    if(tester.currentEvaluationConfig == 4) {                        
                        world.input.testFlythrough = false;

                        tester.Writefile();
                        world.registry.destroy(testerEnt);
                    } else {
                        tester.start = std::chrono::steady_clock::now();
                        tester.currentFrame = 0;

                        if(tester.currentEvaluationConfig >= 2) {
                            world.input.computeTerrain = false;
                        }
                        world.input.previewTriangleDensity = tester.currentEvaluationConfig%2 != 0;
                    }
                }
            }
        } else {
            for(auto entity : testerView) {
                world.registry.destroy(entity);
            }
        }
    }
};