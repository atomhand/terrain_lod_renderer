#pragma once

#include <chrono>
#include "ImGuiProfilerRenderer.h"
#include "world.h"

namespace Engine {
    class Profiler {

        static inline std::vector<legit::ProfilerTask> cpuTasks;
        static inline std::vector<legit::ProfilerTask> gpuTasks;
        static inline ImGuiUtils::ProfilersWindow profilersWindow = ImGuiUtils::ProfilersWindow();

        static inline GLuint gpuTimerObject;

        static inline bool activeGpuTask = false;

        static inline bool gpuProfiling = true;
        static inline bool cpuProfiling = true;

        static inline legit::ProfilerTask* prevGpuTask;

        static inline double lastGpuTiming = 0.0;

        static inline std::vector<uint32_t> colors = {
            legit::Colors::turqoise,
            legit::Colors::emerald ,
            legit::Colors::peterRiver ,
            legit::Colors::amethyst ,
            legit::Colors::sunFlower ,
            legit::Colors::carrot ,
            legit::Colors::alizarin ,
            legit::Colors::clouds ,
        };

        static void FinishPrevGpuTask() {
            if(prevGpuTask != nullptr) {                
                GLuint64 nanoseconds;
                glGetQueryObjectui64v(gpuTimerObject, GL_QUERY_RESULT, &nanoseconds);
                
                double time = double(nanoseconds) / 1.0E9;
                prevGpuTask->endTime = time;
                prevGpuTask = nullptr;
                lastGpuTiming = time;
            }
        }
    public:
        static double GetLastGpuTiming() {
            FinishPrevGpuTask();
            return lastGpuTiming;
        }

        static void Render(Engine::World& world) {
            cpuProfiling = world.input.profilerWindow;
            gpuProfiling = world.input.profilerWindow && world.input.enableGpuProfiling;

            if(world.input.profilerWindow) {
                FinishPrevGpuTask();
                profilersWindow.gpuGraph.LoadFrameData(gpuTasks.data(), gpuTasks.size());
                profilersWindow.cpuGraph.LoadFrameData(cpuTasks.data(), cpuTasks.size());
                profilersWindow.Render();
            }

            gpuTasks.clear();
            cpuTasks.clear();
            prevGpuTask = nullptr;
        }

        struct Handle {
            legit::ProfilerTask* task;
            std::chrono::system_clock::time_point start;

            void End() {
                if(task != nullptr) {
                    auto current_time = std::chrono::system_clock::now();
                    task->endTime = std::chrono::duration<double>(current_time - start).count();
                    task = nullptr;
                }
            }

            ~Handle() {
                End();
            }
        };

        struct GpuHandle {
            legit::ProfilerTask* task;

            void End() {
                if(task != nullptr) {
                    assert(activeGpuTask);
                    activeGpuTask = false;
                    glEndQuery(GL_TIME_ELAPSED);
                    
                    assert(prevGpuTask == nullptr);
                    prevGpuTask = task;
                    task = nullptr;
                }
            }

            ~GpuHandle() {
                End();
            }
        };

        static Handle StartCpu(std::string name, uint32_t color = 0) {
            if(!cpuProfiling) {
                return Handle { nullptr };
            }
            FinishPrevGpuTask();
            if(color == 0) {
                color = colors[cpuTasks.size() % colors.size()];
            }

            cpuTasks.emplace_back (
                0.0,
                0.0,
                name,
                color
            );

            return Handle{
                &cpuTasks.back(),
                std::chrono::system_clock::now()
            };
        }
        
        static GpuHandle StartGpu(std::string name, uint32_t color = 0) {
            if(!gpuProfiling) {
                return GpuHandle { nullptr };
            }
            FinishPrevGpuTask();

            if(color == 0) {
                color = colors[gpuTasks.size() % colors.size()];
            }

            gpuTasks.emplace_back (
                0.0,
                0.0,
                name,
                color
            );

            if(gpuTimerObject == 0) {                
                glGenQueries(1, &gpuTimerObject);
            }

            assert(!activeGpuTask);

            glBeginQuery(GL_TIME_ELAPSED, gpuTimerObject);
            activeGpuTask = true;

            return GpuHandle{
                &gpuTasks.back()
            };
        }
    };
}