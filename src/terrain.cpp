
#include <algorithm>
#include "terrain.h"

void Terrain::SampleRegion(glm::vec2 origin, glm::vec2 step, int cellW, std::span<float> output) {
    noise->GenUniformGrid2D(output.data(), origin.x, origin.y, cellW, cellW, step.x, step.y, config.SEED);
};

// Returns terrain altitude at the given x,z position (world space)
float Terrain::Height(float x, float z) {
    return noise->GenSingle2D(x,z,config.SEED);
}