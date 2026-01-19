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

        static const unsigned int KEYS_PER_THREAD = 8;
        static const unsigned int PARTITION_SIZE = 256 * KEYS_PER_THREAD; 

        static const unsigned int MAX_NUM_BLOCKS = 100000;
        static const unsigned int MAX_NUM = MAX_NUM_BLOCKS * PARTITION_SIZE;

        ComputeShader countKernel = ComputeShader("shaders/algorithm/onesweep_count.cs");
        ComputeShader globalPrefixKernel = ComputeShader("shaders/algorithm/onesweep_global_prefix.cs");
        ComputeShader reorderKernel = ComputeShader("shaders/algorithm/onesweep_reorder.cs");

        StorageBuffer histogram = StorageBuffer(NUM_PASSES * WORD_SIZE * sizeof(unsigned int), 0);
        StorageBuffer blockLocalHistogram = StorageBuffer(WORD_SIZE *  MAX_NUM_BLOCKS * sizeof(unsigned int), 0);

        StorageBuffer scratchBuffer = StorageBuffer(MAX_NUM * sizeof(unsigned int), 0);

        StorageBuffer debugWarpBaseOffset = StorageBuffer(MAX_NUM * sizeof(unsigned int), 0);
        StorageBuffer debugWarpLocalOffset = StorageBuffer(MAX_NUM * sizeof(unsigned int), 0);
        StorageBuffer debugGlobalOffset = StorageBuffer(MAX_NUM * sizeof(unsigned int), 0);


        StorageBuffer blockCounter = StorageBuffer(sizeof(unsigned int), 0);

        void Sort(StorageBuffer& input, StorageBuffer& output, int count, int passes = NUM_PASSES) {
            assert(GLAD_GL_KHR_shader_subgroup);

            input.BindBase(0);
            histogram.BindBase(2);
            blockLocalHistogram.BindBase(3);
            blockCounter.BindBase(4);

            debugWarpBaseOffset.BindBase(5);
            debugWarpLocalOffset.BindBase(6);
            debugGlobalOffset.BindBase(7);

            glClearNamedBufferData(histogram.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            countKernel.use();
            countKernel.setInt("totalCount", count);
            countKernel.Dispatch(std::ceil(count / double(PARTITION_SIZE)), 1, 1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            globalPrefixKernel.Dispatch(NUM_PASSES,1,1);

            int numReorderBlocks = std::ceil(count / double(PARTITION_SIZE));
            assert(numReorderBlocks <= MAX_NUM_BLOCKS);

            reorderKernel.use();
            reorderKernel.setInt("totalCount", count);
            for(int i = 0; i<passes; i++) {
                glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
                int zero = 0;
                glClearNamedBufferSubData(blockLocalHistogram.object(), GL_R8UI, 0, numReorderBlocks*256*sizeof(int), GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
                glClearNamedBufferData(blockCounter.object(), GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero);
                
                if(passes == 1) {                    
                    input.BindBase(0);
                    output.BindBase(1);
                }
                else if(i%2 == 0) {
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
                reorderKernel.use();
                reorderKernel.setInt("currentPass", i);
                reorderKernel.setInt("wordOffset", WORD_BITS * i);
                glDispatchCompute(std::ceil(count / double(PARTITION_SIZE)), 1, 1);
            }
        }
    };

    class GpuSortTester {
    private:
        GpuSort m_Sorter;

        int count = 100000000;
        int countNum = 1;
        int countExp = 6;
        int activeBits = 32;

        std::vector<unsigned int> inputValues;
        std::vector<unsigned int> outputValues;

        std::vector<unsigned int> debugWarpBaseOffset;
        std::vector<unsigned int> debugWarpLocalOffset;
        std::vector<unsigned int> debugGlobalOffset; 


        //std::vector<unsigned int> blockHistograms;
        //std::vector<unsigned int> histogram;

        StorageBuffer inputBuffer = StorageBuffer(GpuSort::MAX_NUM * sizeof(unsigned int));
        StorageBuffer outputBuffer = StorageBuffer(GpuSort::MAX_NUM * sizeof(unsigned int));

        bool sortCorrect;

        int passes = GpuSort::NUM_PASSES;

        int currentBench = 0;
        int benchNumIterations = 64;
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
            inputBuffer.Set((void*)inputValues.data(), sizeof(unsigned int) * inputValues.size(), 0);
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

            auto sortProfileHandle = Profiler::StartGpu("GpuSort");
            m_Sorter.Sort(inputBuffer, outputBuffer, inputValues.size(), passes);
            sortProfileHandle.End();

            if(readback) {
                outputValues.resize(inputValues.size());
                debugWarpBaseOffset.resize(inputValues.size());
                debugWarpLocalOffset.resize(inputValues.size());
                debugGlobalOffset.resize(inputValues.size());
                //histogram.resize(GpuSort::NUM_PASSES * GpuSort::WORD_SIZE);
                //int numBlocks = std::ceil(inputValues.size() / double(GpuSort::PARTITION_SIZE));
                //blockHistograms.resize(numBlocks * GpuSort::WORD_SIZE);
                glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
                //m_Sorter.histogram.Readback<unsigned int>(histogram.data(), GpuSort::NUM_PASSES * GpuSort::WORD_SIZE, 0);
                //m_Sorter.blockLocalHistogram.Readback<unsigned int>(blockHistograms.data(), numBlocks * GpuSort::WORD_SIZE, 0);


                m_Sorter.debugWarpBaseOffset.Readback<unsigned int>(debugWarpBaseOffset.data(), outputValues.size(), 0);
                m_Sorter.debugWarpLocalOffset.Readback<unsigned int>(debugWarpLocalOffset.data(), outputValues.size(), 0);
                m_Sorter.debugGlobalOffset.Readback<unsigned int>(debugGlobalOffset.data(), outputValues.size(), 0);

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
                ImGui::SliderInt("Count Exponent", &countExp, 0, 6);
                count = std::min(double(GpuSort::MAX_NUM),double(countNum * pow(10,countExp)));
                ImGui::Text("Count: %i", count);

                ImGui::SliderInt("Passes", &passes, 1, GpuSort::NUM_PASSES);
                ImGui::SliderInt("Active Bits", &activeBits, 1, 32);

                if(ImGui::CollapsingHeader("Values")) {
                    if(ImGui::BeginTable("valuesTable", 5)) {
                        ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);                
                        ImGui::TableSetupColumn("output", ImGuiTableColumnFlags_WidthStretch); 

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Input");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("Output");
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("GLobal offset");
                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("Warp base offset");
                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("Warp internal offset");

                        int n = std::min(outputValues.size(), (size_t)5000);
                        for(int i =0; i<n; i++) {               
                            ImGui::TableNextRow();                
                            ImGui::TableSetColumnIndex(0);                
                            ImGui::Text("%u", inputValues[i]);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%u", outputValues[i]);

                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%u", debugGlobalOffset[i]);

                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%u", debugWarpBaseOffset[i]);

                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("%u", debugWarpLocalOffset[i]);
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