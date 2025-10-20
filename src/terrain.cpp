
#include <algorithm>
#include "terrain.h"

/*
float Terrain::Plateau(float x, float z) {
    float plateau = 0.f;

    float normalise_sum = 0.f;
    float amp = 1.f;
    float freq = 1.0 / (PERIOD * 4.f);
    // Raising FBM noise to a positive a power produces mountainous shape
    for(int i =0; i<FOOTHILL_OCTAVES; i++) {
        noise.SetSeed(i);
        plateau += noise.GetNoise(x * freq, z * freq) * amp;

        normalise_sum += amp;
        freq *= 2.f;
        amp *= 0.5f;
    }
    plateau /= normalise_sum;
    float fac = 3.0;
    if(plateau > 0.f) plateau = pow(plateau,fac);

    return plateau;
}
*/

glm::vec2 hash22(glm::vec2 x) {
    const glm::vec2 k = glm::vec2( 0.3183099, 0.3678794 );
    x = x * k + glm::vec2(k.y,k.x);
    return -1.0f + 2.0f * glm::fract(16.f * k * glm::fract( x.x * x.y * (x.x + x.y)));
}

glm::vec3 Ridges(glm::vec2 p, glm::vec2 slope) {
    const glm::vec2 sideDir(slope.y * -1.0,slope.x);
    const glm::vec2 ip = glm::floor(p);
    const glm::vec2 fp = glm::fract(p);
    const float revolution = 2.f * glm::pi<float>();

    glm::vec3 va = glm::vec3(0.0);
    float weightSum = 0.0f; // for normalisation

    for(int i=-2; i<=1; i++)
        for(int j=-2; j<=1; j++) {
            glm::vec2 gridOffset(i,j);

            glm::vec2 gridPoint = ip - gridOffset;

            glm::vec2 randomOffset = hash22(gridPoint) * 0.5f;

            // (gridPoint + randomOffset) - p
            // = ((ip - gridOffset) + randomOffset) - (fp + ip)
            // = ip - gridOffset + randomOffset - fp - ip
            // = randomOffset - gridOffset - fp
            glm::vec2 vectorToCellPoint = randomOffset - gridOffset - fp;

            // Bell-shaped weight function which is 1 at dist 0 and nearly 0 at dist 1.5.
            float sqrDist = glm::dot(vectorToCellPoint,vectorToCellPoint);
            float weight = std::exp(-sqrDist * 2.f);

            weightSum += weight;

            float waveInput = glm::dot(vectorToCellPoint, sideDir) * revolution;

            va += glm::vec3(std::cos(waveInput), std::sin(waveInput) * sideDir) * weight;
        }
    
    return va / weightSum;
}

#define EROSION_GAIN 0.5f
#define EROSION_LACUNARITY 2.0f

#define EROSION_SLOPE_STRENGTH 3.0f
#define EROSION_SLOPE_SENSITIVITY 0.3f

#define EROSION_BRANCH_STRENGTH 3.0f

float Terrain::Erosion(glm::vec2 p, float h, glm::vec3 normal, float a, int octaves, float& maxErosion) {
    glm::vec3 heightAndSlope(h,-normal.x,-normal.z);
    float slopeMag = glm::length(glm::vec2(normal.x,normal.z));
    float slopeScale = 1.0f / std::lerp(slopeMag, 1.0f / EROSION_SLOPE_STRENGTH, EROSION_SLOPE_SENSITIVITY);
    heightAndSlope *= glm::vec3(1.f,slopeScale,slopeScale);

    float f = 1.f;
    for(int i=0; i<octaves; i++) {
        float old = heightAndSlope.x;

        glm::vec3 ridges = Ridges(p* f, glm::vec2(heightAndSlope.y,heightAndSlope.z));

        heightAndSlope += ridges
            * a * glm::vec3(1.f, f * EROSION_BRANCH_STRENGTH, f * EROSION_BRANCH_STRENGTH);
        a *= EROSION_GAIN;
        f *= EROSION_LACUNARITY;

        maxErosion = std::max(maxErosion, -ridges.x);
    }

    return heightAndSlope.x;
}

void Terrain::SampleRegion(glm::vec2 origin, glm::vec2 step, int cellW, std::span<float> output, std::span<glm::vec2> extraOutput) {
    int tempW = cellW+2;
    heightTemp.assign(tempW*tempW, 0.f);

    noise->GenUniformGrid2D(heightTemp.data(), origin.x - step.x, origin.y - step.y, tempW, tempW, step.x, step.y, config.SEED);

    // Skip erosion for chunks with a negligible visual contribution
    // Current parameters cull quite aggressively, which works fine for shallow viewing angle of the world (ie. standing on it)
    // but badly from high in the air
    // TODO
    // Skip octaves according to LoD error 
    float eFactor = 100.0 * config.EROSION_STRENGTH; // 20 is empirical constant
    float scaleOverError = eFactor * config.EROSION_SCALE* config.FINAL_SCALE / (step.x * 1.4f);
    bool doErosion = config.erosionEnabled && scaleOverError > 2.f;

    for(int z=0; z<cellW; z++) {
        for(int x=0; x<cellW; x++) {
            int tX = x+1;
            int tZ = z+1;
            float height = heightTemp[tX + tZ * tempW];

            glm::vec2 erosionAmount = glm::vec2(0.f);

            float a = config.EROSION_SCALE * 0.5f;//+ glm::smoothstep(-0.5f * config.foothillsScale, config.foothillsScale, height);
            if(a > 0.f && doErosion) {
                glm::vec2 center = origin + glm::vec2(step.x * x, step.y * z);
                // calculate normal            
                glm::vec3 L = glm::vec3(center.x-step.x, heightTemp[(tX-1) + tZ * tempW] * config.FINAL_SCALE, center.y);
                glm::vec3 R = glm::vec3(center.x+step.x, heightTemp[(tX+1) + tZ * tempW] * config.FINAL_SCALE, center.y);
                glm::vec3 U = glm::vec3(center.x,       heightTemp[tX + (tZ-1) * tempW] * config.FINAL_SCALE, center.y-step.y);
                glm::vec3 D = glm::vec3(center.x,       heightTemp[tX + (tZ+1) * tempW] * config.FINAL_SCALE, center.y+step.y);
                glm::vec3 normal = glm::normalize(glm::cross(R-L,U-D));

                float erosion = Erosion(center / config.EROSION_PERIOD, height, normal, a, config.EROSION_OCTAVES, erosionAmount.x);
                float newHeight = height + (erosion - height - 0.5f)  * config.EROSION_STRENGTH;//height + (erosion - height) * EROSION_STRENGTH;
                erosionAmount.y = -erosion;
                height = newHeight;
            }

            output[x + z * cellW] = height * config.FINAL_SCALE + 4.f;
            extraOutput[x + z * cellW] = erosionAmount;
        }
    }
};

// Returns terrain altitude at the given x,z position (world space)
float Terrain::Height(float x, float z) {
    float h[1];
    glm::vec2 e[1];
    SampleRegion(glm::vec2(x,z), glm::vec2(0.1f), 1, h, e);
    return h[0];
}