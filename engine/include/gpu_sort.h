#pragma once

#include <random>
#include <algorithm>

#include "world.h"
#include "compute_shader.h"
#include "storage_buffer.h"
#include "imgui.h"

#include "profiler.h"

namespace Engine {   
    class GpuSort {
    private:  
    public:  
        static const unsigned int WORD_BITS = 8u;
        static const unsigned int WORD_SIZE = 256u;
        static const unsigned int NUM_PASSES = 4;
        static const unsigned int WORD_MASK = 0xFFu;
        static const unsigned int HISTOGRAM_SIZE = NUM_PASSES * WORD_SIZE;

        const unsigned int KEYS_PER_THREAD;
        const unsigned int PARTITION_SIZE; 

        static const unsigned int MAX_NUM_BLOCKS = 100000;
        const unsigned int MAX_NUM;

        ComputeShader countKernel;
        ComputeShader globalPrefixKernel;
        ComputeShader reorderKernel;

        unsigned int activeHisto = 0;
        StorageBuffer histogram[2] = { StorageBuffer(NUM_PASSES * WORD_SIZE * sizeof(unsigned int), 0), StorageBuffer(NUM_PASSES * WORD_SIZE * sizeof(unsigned int), 0) };
        StorageBuffer blockLocalHistogram = StorageBuffer(WORD_SIZE *  MAX_NUM_BLOCKS * sizeof(unsigned int), 0);

        StorageBuffer scratchBuffer = StorageBuffer(MAX_NUM * sizeof(unsigned int), 0);

        StorageBuffer blockCounter = StorageBuffer(sizeof(unsigned int), 0);

        GLuint countKernelCountLocation;
        GLuint reorderKernelCountLocation;
        GLuint reorderKernelCurrentPassLocation;
        GLuint reorderKernelWordOffsetLocation;
        GLuint reorderKernelBlockCountLocation;

        GLuint countProgram;
        GLuint reorderProgram;
        GLuint prefixProgram;

        GpuSort(unsigned int keysPerThread) : KEYS_PER_THREAD(keysPerThread),
            PARTITION_SIZE(256 * KEYS_PER_THREAD),
            MAX_NUM(MAX_NUM_BLOCKS * PARTITION_SIZE)        
        {
            std::string def = std::string("#define KEYS_PER_THREAD ") + std::to_string(keysPerThread);
            std::vector<const char*> defs = std::vector<const char*>{def.c_str()};

            countKernel = ComputeShader("shaders/algorithm/onesweep_count.cs", defs);
            globalPrefixKernel = ComputeShader("shaders/algorithm/onesweep_global_prefix.cs", defs);
            reorderKernel = ComputeShader("shaders/algorithm/onesweep_reorder.cs", defs);

            countProgram = countKernel.programId();
            reorderProgram = reorderKernel.programId();
            prefixProgram = globalPrefixKernel.programId();

            countKernelCountLocation = glGetUniformLocation(countKernel.programId(), "totalCount");
            reorderKernelCountLocation = glGetUniformLocation(reorderKernel.programId(), "totalCount");
            reorderKernelCurrentPassLocation = glGetUniformLocation(reorderKernel.programId(), "currentPass");
            reorderKernelWordOffsetLocation = glGetUniformLocation(reorderKernel.programId(), "wordOffset");
            reorderKernelBlockCountLocation = glGetUniformLocation(reorderKernel.programId(), "blocksPerPass");
            
            glClearNamedBufferData(blockLocalHistogram.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
            glClearNamedBufferData(histogram[0].object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
            glClearNamedBufferData(histogram[1].object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        }

        void Sort(StorageBuffer& input, StorageBuffer& output, int count, int passes = NUM_PASSES) {
            assert(GLAD_GL_KHR_shader_subgroup);

            input.BindBase(0);
            output.BindBase(1);

            histogram[activeHisto].BindBase(2);
            histogram[(activeHisto+1)%2].BindBase(5);

            activeHisto = (activeHisto+1)%2;

            blockLocalHistogram.BindBase(3);
            blockCounter.BindBase(4);

            unsigned int dispatchNumBlocks = (count+PARTITION_SIZE-1)/PARTITION_SIZE;
            assert(dispatchNumBlocks <= MAX_NUM_BLOCKS);

            glUseProgram(countProgram);
            glUniform1i(countKernelCountLocation, count);
            glDispatchCompute(dispatchNumBlocks,1,1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            glUseProgram(prefixProgram);
            glDispatchCompute(1,1,1);
            
            glUseProgram(reorderProgram);            
            glUniform1i(reorderKernelCountLocation, count);
            glUniform1i(reorderKernelBlockCountLocation, dispatchNumBlocks);
            for(int i = 0; i<passes; i++) {                
                if(i%2 == 0) {
                    if(i == 0)
                        input.BindBase(0);
                    else
                        output.BindBase(0);
                    scratchBuffer.BindBase(1);
                } else {                        
                    scratchBuffer.BindBase(0);
                    output.BindBase(1);
                }
                
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                
                glDispatchCompute(dispatchNumBlocks, 1, 1);
            }
        }
    };

    class GpuSortTester {
    private:
        std::unique_ptr<GpuSort> m_Sorter;

        int count;
        int countNum = 10;
        int countExp = 6;
        int activeBits = 32;

        std::vector<unsigned int> inputValues;
        std::vector<unsigned int> outputValues;

        //std::vector<unsigned int> blockHistograms;
        //std::vector<unsigned int> histogram;

        StorageBuffer inputBuffer = StorageBuffer(0);
        StorageBuffer outputBuffer = StorageBuffer(0);

        bool sortCorrect;

        int passes = GpuSort::NUM_PASSES;

        int currentBench = 0;
        int benchNumIterations = 64;
        int keysPerThread = 8;
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

                double time = TestSort(false);
                double rate = count / time;

                sumRate += rate;
                totalBenchTime += time;
                avgRate = sumRate / double(benchNumIterations - currentBench);
            }
        }

        double TestSort(bool readback) {
            inputBuffer.Resize<unsigned int>(inputValues.size());
            outputBuffer.Resize<unsigned int>(inputValues.size());
            inputBuffer.Set((void*)inputValues.data(), sizeof(unsigned int) * inputValues.size(), 0);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

            auto sortProfileHandle = Profiler::StartGpu("GpuSort");
            m_Sorter->Sort(inputBuffer, outputBuffer, inputValues.size(), passes);
            sortProfileHandle.End();

            if(readback) {
                outputValues.resize(inputValues.size());
                glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
                outputBuffer.Readback<unsigned int>(outputValues.data(), outputValues.size(), 0);
                sortCorrect = CheckSortResult();
            }

            return Profiler::GetLastGpuTiming();
        }

        bool CheckSortResult() {
            std::vector<unsigned int> sorted = std::vector<unsigned int>(inputValues);

            std::sort(sorted.begin(),sorted.end());
            
            for(int i = 0; i<outputValues.size(); i++) {
                if(outputValues[i] != sorted[i]) {
                    return false;
                }
            }
            return true;
        }
        std::mt19937 gen32;
    public:        
        GpuSortTester() {
            m_Sorter = std::make_unique<GpuSort>((unsigned int)keysPerThread);
            //histogram.resize(GpuSort::NUM_PASSES * GpuSort::WORD_SIZE);

            //for(int i =0; i<GpuSort::NUM_PASSES * GpuSort::WORD_SIZE; i++)
            //    histogram[i] = i;
        }

        void GenerateInputValues(int n) {            
            inputValues.clear();
            unsigned int mask = 0xFFFFFFFFu >> (32 - activeBits);
            
            std::uniform_int_distribution<unsigned int> numbers_dist = std::uniform_int_distribution<unsigned int>(0, mask);
            for(int i =0; i<count; i++) {
                inputValues.push_back(numbers_dist(gen32));
            }
        }

        void DrawInterface(World& world) {
            if(ImGui::Begin("Onesweep Radix Sort Tester")) {
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
                if(ImGui::Button("Sort")) {
                    TestSort(true);
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
                count = std::min(double(m_Sorter->MAX_NUM),double(countNum * pow(10,countExp)));
                ImGui::Text("Count: %i", count);

                
                ImGui::SliderInt("Keys per thread", &keysPerThread, 1, 32);
                if(keysPerThread != m_Sorter->KEYS_PER_THREAD) {                    
                    m_Sorter = std::make_unique<GpuSort>((unsigned int)keysPerThread);
                }

                ImGui::SliderInt("Passes", &passes, 1, GpuSort::NUM_PASSES);
                ImGui::SliderInt("Active Bits", &activeBits, 1, 32);

                if(ImGui::CollapsingHeader("Values")) {
                    if(ImGui::BeginTable("valuesTable", 2)) {
                        ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);                
                        ImGui::TableSetupColumn("output", ImGuiTableColumnFlags_WidthStretch); 

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Input");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("Output");

                        int n = std::min(outputValues.size(), (size_t)5000);
                        for(int i =0; i<n; i++) {               
                            ImGui::TableNextRow();                
                            ImGui::TableSetColumnIndex(0);                
                            ImGui::Text("%u", inputValues[i]);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%u", outputValues[i]);
                        }

                        ImGui::EndTable();
                    }
                }
                /*
                if(ImGui::CollapsingHeader("Histogram")) {  
                    if(ImGui::BeginTable("histogramTable", 1 + GpuSort::NUM_PASSES)) {                
                        ImGui::TableNextRow();               
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Value");
                        
                        for(int p =0; p<GpuSort::NUM_PASSES; p++) {
                            ImGui::TableSetColumnIndex(p+1);
                            ImGui::Text("P%i", p);
                        }
                        
                        for(int i =0; i<GpuSort::WORD_SIZE; i++) {                        
                            ImGui::TableNextRow();                    
                            ImGui::TableSetColumnIndex(0);                        
                            ImGui::Text("H%i", i);
                            for(int p =0; p<GpuSort::NUM_PASSES; p++) {
                                ImGui::TableSetColumnIndex(p+1);
                                ImGui::Text("%i", histogram[i + p * GpuSort::WORD_SIZE]);
                            }
                        }

                        ImGui::EndTable();
                    }
                }

                ImGui::Separator();

                if(ImGui::CollapsingHeader("Block Histograms")) {
                    int numBlocks = blockHistograms.size() / GpuSort::WORD_SIZE;                
                    if(ImGui::BeginTable("BlockHistogramsTable", 1 + numBlocks)) { 
                        ImGui::TableNextRow();               
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Value");

                        for(int b =0; b<numBlocks; b++) {
                            ImGui::TableSetColumnIndex(b+1);
                            ImGui::Text("B%i", b);
                        }
                        
                        for(int i =0; i<GpuSort::WORD_SIZE; i++) {                    
                            ImGui::TableNextRow();                    
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("H%i", i);
                            for(int b =0; b<numBlocks; b++) {
                                ImGui::TableSetColumnIndex(b+1);
                                ImGui::Text("%i", blockHistograms[i + b * GpuSort::WORD_SIZE]);
                            }
                        }

                        ImGui::EndTable();
                    }
                }
                */
            }
            ImGui::End();
        }
    };
}