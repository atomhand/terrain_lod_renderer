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

        const unsigned int REORDER_WARPS;
        const unsigned int KEYS_PER_THREAD;
        const unsigned int PARTITION_SIZE; 

        static const unsigned int MAX_NUM_BLOCKS = 100000;
        const unsigned int MAX_NUM;

        ComputeShader countKernel;
        ComputeShader globalPrefixKernel;
        ComputeShader reorderKernel;
        
        ComputeShader countKernelPaired;
        ComputeShader globalPrefixKernelPaired;
        ComputeShader reorderKernelPaired;

        unsigned int activeHisto = 0;
        StorageBuffer histogram[2] = { StorageBuffer(NUM_PASSES * WORD_SIZE * sizeof(unsigned int), 0), StorageBuffer(NUM_PASSES * WORD_SIZE * sizeof(unsigned int), 0) };
        StorageBuffer blockLocalHistogram = StorageBuffer(WORD_SIZE *  MAX_NUM_BLOCKS * sizeof(unsigned int), 0);

        //StorageBuffer scratchBuffer = StorageBuffer(MAX_NUM * sizeof(unsigned int), 0);

        StorageBuffer blockCounter = StorageBuffer(sizeof(unsigned int), 0);

        GLuint countKernelCountLocation;
        GLuint reorderKernelCountLocation;
        GLuint reorderKernelBlockCountLocation;

        GLuint countKernelPairedCountLocation;
        GLuint reorderKernelPairedCountLocation;
        GLuint reorderKernelPairedBlockCountLocation;

        GpuSort(unsigned int keysPerThread = 12, unsigned int reorderWarps = 8) : KEYS_PER_THREAD(keysPerThread),
            REORDER_WARPS(reorderWarps),
            PARTITION_SIZE(REORDER_WARPS * 32 * KEYS_PER_THREAD),
            MAX_NUM(MAX_NUM_BLOCKS * PARTITION_SIZE)        
        {
            std::string def = std::string("#define KEYS_PER_THREAD ") + std::to_string(keysPerThread);
            std::string def2 = std::string("#define NUM_WARPS ") + std::to_string(REORDER_WARPS);
            std::vector<const char*> defs = std::vector<const char*>{def.c_str(), def2.c_str()};

            countKernel = ComputeShader("shaders/algorithm/onesweep_count.cs", defs);
            globalPrefixKernel = ComputeShader("shaders/algorithm/onesweep_global_prefix.cs", defs);
            reorderKernel = ComputeShader("shaders/algorithm/onesweep_reorder.cs", defs);

            defs.push_back("#define PAIRED");
            countKernelPaired = ComputeShader("shaders/algorithm/onesweep_count.cs", defs);
            globalPrefixKernelPaired = ComputeShader("shaders/algorithm/onesweep_global_prefix.cs", defs);
            reorderKernelPaired = ComputeShader("shaders/algorithm/onesweep_reorder.cs", defs);

            countKernelCountLocation = glGetUniformLocation(countKernel.programId(), "totalCount");
            reorderKernelCountLocation = glGetUniformLocation(reorderKernel.programId(), "totalCount");
            reorderKernelBlockCountLocation = glGetUniformLocation(reorderKernel.programId(), "blocksPerPass");

            countKernelPairedCountLocation = glGetUniformLocation(countKernelPaired.programId(), "totalCount");
            reorderKernelPairedCountLocation = glGetUniformLocation(reorderKernelPaired.programId(), "totalCount");
            reorderKernelPairedBlockCountLocation = glGetUniformLocation(reorderKernelPaired.programId(), "blocksPerPass");
            
            glClearNamedBufferData(blockLocalHistogram.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
            glClearNamedBufferData(histogram[0].object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
            glClearNamedBufferData(histogram[1].object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        }

        void SortInPlacePaired(StorageBuffer& input, StorageBuffer& scratch, int count) {
            
            assert(GLAD_GL_KHR_shader_subgroup);

            input.BindBase(0);
            scratch.BindBase(1);

            histogram[activeHisto].BindBase(2);
            histogram[(activeHisto+1)%2].BindBase(5);

            activeHisto = (activeHisto+1)%2;

            blockLocalHistogram.BindBase(3);
            blockCounter.BindBase(4);

            unsigned int dispatchNumBlocks = (count+PARTITION_SIZE-1)/PARTITION_SIZE;
            assert(dispatchNumBlocks <= MAX_NUM_BLOCKS);

            glUseProgram(countKernelPaired.programId());
            glUniform1i(countKernelPairedCountLocation, count);
            glDispatchCompute(dispatchNumBlocks,1,1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            glUseProgram(globalPrefixKernelPaired.programId());
            glDispatchCompute(NUM_PASSES,1,1);
            
            glUseProgram(reorderKernelPaired.programId());            
            glUniform1i(reorderKernelPairedCountLocation, count);
            glUniform1i(reorderKernelPairedBlockCountLocation, dispatchNumBlocks);
            for(int i = 0; i<NUM_PASSES; i++) {                
                if(i%2 == 0) {
                    input.BindBase(0);
                    scratch.BindBase(1);
                } else {
                    scratch.BindBase(0);
                    input.BindBase(1);
                }
                
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);                
                glDispatchCompute(dispatchNumBlocks, 1, 1);
            }
        }

        void SortInPlace(StorageBuffer& input, StorageBuffer& scratch, int count) {
            
            assert(GLAD_GL_KHR_shader_subgroup);

            input.BindBase(0);
            scratch.BindBase(1);

            histogram[activeHisto].BindBase(2);
            histogram[(activeHisto+1)%2].BindBase(5);

            activeHisto = (activeHisto+1)%2;

            blockLocalHistogram.BindBase(3);
            blockCounter.BindBase(4);

            unsigned int dispatchNumBlocks = (count+PARTITION_SIZE-1)/PARTITION_SIZE;
            assert(dispatchNumBlocks <= MAX_NUM_BLOCKS);

            glUseProgram(countKernel.programId());
            glUniform1i(countKernelCountLocation, count);
            glDispatchCompute(dispatchNumBlocks,1,1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            glUseProgram(globalPrefixKernel.programId());
            glDispatchCompute(NUM_PASSES,1,1);
            
            glUseProgram(reorderKernel.programId());            
            glUniform1i(reorderKernelCountLocation, count);
            glUniform1i(reorderKernelBlockCountLocation, dispatchNumBlocks);
            for(int i = 0; i<NUM_PASSES; i++) {                
                if(i%2 == 0) {
                    input.BindBase(0);
                    scratch.BindBase(1);
                } else {
                    scratch.BindBase(0);
                    input.BindBase(1);
                }
                
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);                
                glDispatchCompute(dispatchNumBlocks, 1, 1);
            }
        }
/*
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
            glDispatchCompute(NUM_PASSES,1,1);
            
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
    */
        };

    class GpuSortTester {
    private:
        std::unique_ptr<GpuSort> m_Sorter;

        int count;
        int countNum = 10;
        int countExp = 6;
        int activeBits = 32;

        std::vector<unsigned int> inputValues;
        std::vector<glm::uvec2> inputValuesPaired;
        std::vector<unsigned int> outputValues;

        //std::vector<unsigned int> blockHistograms;
        //std::vector<unsigned int> histogram;

        StorageBuffer inputBuffer = StorageBuffer(0);
        StorageBuffer scratchBuffer = StorageBuffer(0);

        bool sortCorrect;

        int currentBench = 0;
        int benchNumIterations = 64;
        int keysPerThread = 12;
        int warpsPerBlock = 8;
        double sumRate = 0.0;
        double avgRate = 0.0;
        double totalBenchTime = 0.0;

        bool nextSortPaired = false;
        bool lastSortPaired = false;

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
            uint32_t n = nextSortPaired ? inputValuesPaired.size() * 2 : inputValues.size();
            inputBuffer.Resize<unsigned int>(n);
            scratchBuffer.Resize<unsigned int>(n);
            if(nextSortPaired) {
                inputBuffer.Set<glm::uvec2>(inputValuesPaired.data(), inputValuesPaired.size(), 0);
            } else {
                inputBuffer.Set<unsigned int>(inputValues.data(), inputValues.size(), 0);
            }
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

            auto sortProfileHandle = Profiler::StartGpu("GpuSort");
            if(nextSortPaired) {
                m_Sorter->SortInPlacePaired(inputBuffer,scratchBuffer,inputValuesPaired.size());
            } else {                
                m_Sorter->SortInPlace(inputBuffer, scratchBuffer, inputValues.size());
            }
            sortProfileHandle.End();

            if(readback) {
                lastSortPaired = nextSortPaired;

                outputValues.resize(n);
                glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
                inputBuffer.Readback<unsigned int>(outputValues.data(), outputValues.size(), 0);
                sortCorrect = CheckSortResult();
            }

            return Profiler::GetLastGpuTiming();
        }

        static bool pairedComparison(glm::uvec2 a, glm::uvec2 b) {
            return a.x < b.x;
        }

        bool pairedValueCorrect;
        bool CheckSortResult() {
            pairedValueCorrect = true;
            
            std::vector<unsigned int> sorted = std::vector<unsigned int>(inputValues);
            std::sort(sorted.begin(),sorted.end());

            if(lastSortPaired) {
                for(int i = 0; i<sorted.size(); i++) {
                    if(outputValues[i*2] != sorted[i]) {
                        return false;
                    }
                    if(outputValues[i*2] != inputValues[outputValues[i*2+1]]) {
                        pairedValueCorrect = false;
                    }
                }
            } else {
                for(int i = 0; i<sorted.size(); i++) {
                    if(outputValues[i] != sorted[i]) {
                        return false;
                    }
                }
            }
            return true;
        }
        std::mt19937 gen32;
    public:        
        GpuSortTester() {
            m_Sorter = std::make_unique<GpuSort>((unsigned int)keysPerThread,(unsigned int)warpsPerBlock);
            //histogram.resize(GpuSort::NUM_PASSES * GpuSort::WORD_SIZE);

            //for(int i =0; i<GpuSort::NUM_PASSES * GpuSort::WORD_SIZE; i++)
            //    histogram[i] = i;
        }

        void GenerateInputValues(int n) {            
            inputValues.clear();
            inputValuesPaired.clear();
            unsigned int mask = 0xFFFFFFFFu >> (32 - activeBits);
            
            std::uniform_int_distribution<unsigned int> numbers_dist = std::uniform_int_distribution<unsigned int>(0, mask);
            for(int i =0; i<count; i++) {
                inputValues.push_back(numbers_dist(gen32));
                inputValuesPaired.push_back(glm::uvec2(inputValues[i],i));
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
                if(lastSortPaired) {                    
                    ImGui::Text(pairedValueCorrect ? "Paired values Correct" : "Paired values Not Correct");
                }

                ImGui::SliderInt("CountNum", &countNum, 1, 100);
                ImGui::SliderInt("Count Exponent", &countExp, 0, 7);
                count = std::min(double(m_Sorter->MAX_NUM),double(countNum * pow(10,countExp)));
                ImGui::Text("Count: %i", count);

                ImGui::Checkbox("Paired sort", &nextSortPaired);

                
                ImGui::SliderInt("Keys per thread", &keysPerThread, 1, 32);
                ImGui::SliderInt("Warps per block", &warpsPerBlock, 8, 32);
                if(keysPerThread != m_Sorter->KEYS_PER_THREAD || warpsPerBlock != m_Sorter->REORDER_WARPS) {                    
                    m_Sorter = std::make_unique<GpuSort>((unsigned int)keysPerThread,(unsigned int)warpsPerBlock);
                }

                ImGui::SliderInt("Active Bits", &activeBits, 1, 32);

                if(ImGui::CollapsingHeader("Values")) {
                    if(ImGui::BeginTable("valuesTable", 3)) {
                        ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);                
                        ImGui::TableSetupColumn("output", ImGuiTableColumnFlags_WidthStretch);              
                        ImGui::TableSetupColumn("pairedValues", ImGuiTableColumnFlags_WidthStretch); 

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Input");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("Output");

                        if(lastSortPaired) {
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("Paired Values");
                        }

                        int n = std::min(lastSortPaired ? outputValues.size() / 2 : outputValues.size(), (size_t)5000);
                        for(int i =0; i<n; i++) {               
                            ImGui::TableNextRow();                
                            ImGui::TableSetColumnIndex(0);                
                            ImGui::Text("%u", inputValues[i]);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%u", outputValues[lastSortPaired ? i*2 : i]);

                            if(lastSortPaired) {
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%u", outputValues[i*2+1]);
                            }
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