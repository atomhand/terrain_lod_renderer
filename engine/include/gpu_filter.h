#pragma once

#include <random>
#include <algorithm>

#include "world.h"
#include "compute_shader.h"
#include "storage_buffer.h"
#include "imgui.h"

#include "profiler.h"

namespace Engine {   
    class GpuFilter {
    private:  
    public:
        const unsigned int NUM_WARPS;
        const unsigned int KEYS_PER_THREAD;
        const unsigned int PARTITION_SIZE;

        static const unsigned int MAX_NUM_BLOCKS = 100000;
        const unsigned int MAX_NUM;

        ComputeShader kernel;

        StorageBuffer blockCounts = StorageBuffer(MAX_NUM_BLOCKS * sizeof(unsigned int), 0);
        StorageBuffer scratchBuffer = StorageBuffer(MAX_NUM * sizeof(unsigned int), 0);
        StorageBuffer blockCounter = StorageBuffer(sizeof(unsigned int), 0);

        GLuint kernelCountLocation;

        GLuint program;

        GpuFilter(unsigned int keysPerThread, unsigned int numWarps) : KEYS_PER_THREAD(keysPerThread),
            NUM_WARPS(numWarps),
            PARTITION_SIZE(NUM_WARPS * 32 * KEYS_PER_THREAD),
            MAX_NUM(MAX_NUM_BLOCKS * PARTITION_SIZE)        
        {
            std::string def = std::string("#define KEYS_PER_THREAD ") + std::to_string(keysPerThread);
            std::string def2 = std::string("#define NUM_WARPS ") + std::to_string(NUM_WARPS);
            std::vector<const char*> defs = std::vector<const char*>{def.c_str(), def2.c_str()};

            kernel = ComputeShader("shaders/algorithm/filter.cs", defs);

            program = kernel.programId();

            kernelCountLocation = glGetUniformLocation(kernel.programId(), "totalCount");
            
            glClearNamedBufferData(blockCounts.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        }

        void Filter(StorageBuffer& input, StorageBuffer& output, int count) {
            assert(GLAD_GL_KHR_shader_subgroup);

            glClearNamedBufferData(blockCounts.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
            glClearNamedBufferData(blockCounter.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

            input.BindBase(0);
            output.BindBase(1);
            blockCounts.BindBase(2);
            blockCounter.BindBase(3);

            unsigned int dispatchNumBlocks = (count+PARTITION_SIZE-1)/PARTITION_SIZE;
            assert(dispatchNumBlocks <= MAX_NUM_BLOCKS);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            glUseProgram(program);
            glUniform1i(kernelCountLocation, count);
            glDispatchCompute(dispatchNumBlocks,1,1);
        }
    };

    class GpuFilterTester {
    private:
        std::unique_ptr<GpuFilter> m_Filter;

        int count;
        int countNum = 10;
        int countExp = 6;

        std::vector<unsigned int> inputValues;
        std::vector<unsigned int> outputValues;

        StorageBuffer inputBuffer = StorageBuffer(0);
        StorageBuffer outputBuffer = StorageBuffer(0);

        bool sortCorrect;

        int currentBench = 0;
        int benchNumIterations = 64;
        int keysPerThread = 8;
        int warpsPerBlock = 8;
        double sumRate = 0.0;
        double avgRate = 0.0;
        double totalBenchTime = 0.0;

        void StartBench() {
            currentBench = benchNumIterations;
            sumRate = 0.0;
            avgRate = 0.0;
            totalBenchTime = 0.0;
        }

        void IterBench() {
            if(currentBench > 0) {
                currentBench--;
                GenerateInputValues(count);

                double time = TestFilter(false);
                double rate = count / time;

                sumRate += rate;
                totalBenchTime += time;
                avgRate = sumRate / double(benchNumIterations - currentBench);
            }
        }

        double TestFilter(bool readback) {
            inputBuffer.Resize<unsigned int>(inputValues.size());
            outputBuffer.Resize<unsigned int>(inputValues.size());
            inputBuffer.Set((void*)inputValues.data(), sizeof(unsigned int) * inputValues.size(), 0);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

            auto sortProfileHandle = Profiler::StartGpu("GpuSort");
            m_Filter->Filter(inputBuffer, outputBuffer, inputValues.size());
            sortProfileHandle.End();

            if(readback) {
                outputValues.resize(inputValues.size());
                glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
                outputBuffer.Readback<unsigned int>(outputValues.data(), outputValues.size(), 0);
                sortCorrect = CheckFilterResult();
            }

            return Profiler::GetLastGpuTiming();
        }

        bool FilterOp(unsigned int key) {
            return key % 2 == 0;
        }

        bool CheckFilterResult() {            
            int j = 0;
            for(int i = 0; i<inputValues.size(); i++) {
                if(!FilterOp(inputValues[i])) continue;

                if(outputValues[j] != inputValues[i]) {
                    return false;
                }
                j++;
            }
            return true;
        }
        std::mt19937 gen32;
    public:        
        GpuFilterTester() {
            m_Filter = std::make_unique<GpuFilter>((unsigned int)keysPerThread,(unsigned int)warpsPerBlock);
        }

        void GenerateInputValues(int n) {            
            inputValues.clear();
            unsigned int max = 0xffffffff;
            
            std::uniform_int_distribution<unsigned int> numbers_dist = std::uniform_int_distribution<unsigned int>(0, max);
            for(int i =0; i<count; i++) {
                inputValues.push_back(numbers_dist(gen32));
            }
        }

        void DrawInterface(World& world) {
            if(ImGui::Begin("Gpu Filter Tester")) {
                int subGroupSize;
                glGetIntegerv(GL_SUBGROUP_SIZE_KHR, &subGroupSize);
                ImGui::Text("Subgroup size %i", subGroupSize);

                int supportedFeatures;
                glGetIntegerv(GL_SUBGROUP_SUPPORTED_FEATURES_KHR, &supportedFeatures);
                if(!(supportedFeatures & GL_SUBGROUP_FEATURE_BALLOT_BIT_KHR)) {                    
                    ImGui::Text("Subgroup ballot not supported!");
                }

                if(ImGui::Button("Generate Input Data")) {
                    GenerateInputValues(count);
                }
                if(ImGui::Button("Filter")) {
                    TestFilter(true);
                }

                IterBench();
                ImGui::SliderInt("Benchmark Iterations", &benchNumIterations, 16, 512);
                if(ImGui::Button("Benchmark")) {
                    StartBench();
                }

                if(avgRate > 1.0e9) {                    
                    ImGui::Text("Benchmark: %fB/s (%i iterations completed in %f)", avgRate / 1.0e9, benchNumIterations - currentBench, totalBenchTime);
                } else if(avgRate > 1.06) {
                    ImGui::Text("Benchmark: %fM/s (%i iterations completed in %f)", avgRate / 1.0e6, benchNumIterations - currentBench, totalBenchTime);
                } else {
                    ImGui::Text("Benchmark: %f/s (%i iterations completed in %f)", avgRate, benchNumIterations - currentBench, totalBenchTime);
                }
                
                ImGui::Text(sortCorrect ? "Sort Correct" : "Sort Not Correct");

                ImGui::SliderInt("CountNum", &countNum, 1, 100);
                ImGui::SliderInt("Count Exponent", &countExp, 0, 7);
                count = std::min(double(m_Filter->MAX_NUM),double(countNum * pow(10,countExp)));
                ImGui::Text("Count: %i", count);

                
                ImGui::SliderInt("Keys per thread", &keysPerThread, 1, 32);
                ImGui::SliderInt("Warps per block", &warpsPerBlock, 8, 32);
                if(keysPerThread != m_Filter->KEYS_PER_THREAD || warpsPerBlock != m_Filter->NUM_WARPS) {                    
                    m_Filter = std::make_unique<GpuFilter>((unsigned int)keysPerThread,(unsigned int)warpsPerBlock);
                }

                if(ImGui::CollapsingHeader("Values")) {
                    if(ImGui::BeginTable("valuesTable", 3)) {
                        ImGui::TableSetupColumn("i", ImGuiTableColumnFlags_WidthStretch);    
                        ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);                
                        ImGui::TableSetupColumn("output", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("i");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("Input");
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("Output");

                        int n = std::min(outputValues.size(), (size_t)5000);
                        for(int i =0; i<n; i++) {               
                            ImGui::TableNextRow();               
                            ImGui::TableSetColumnIndex(0);                
                            ImGui::Text("%u", i);

                            ImGui::TableSetColumnIndex(1);                
                            ImGui::Text("%u", inputValues[i]);

                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%u", outputValues[i]);
                        }

                        ImGui::EndTable();
                    }
                }
            }
            ImGui::End();
        }
    };
}