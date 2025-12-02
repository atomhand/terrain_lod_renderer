#pragma once
#include <vector>
#include <limits>
#include "glm/glm.hpp"

namespace Engine {
    struct AABB {
    public:
        glm::vec3 min;
        glm::vec3 max;

        AABB(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}

        AABB() {
            min = glm::vec3(999999.f);
            max = glm::vec3(-999999.f);
        }

        AABB(std::vector<glm::vec3> points) {
            min = glm::vec3(std::numeric_limits<float>::max());
            max = glm::vec3(std::numeric_limits<float>::min());

            for(auto point : points)  {
                min = glm::min(min,point);
                max = glm::max(max,point);
            }
        }

        static AABB Combine(AABB a, AABB b) {
            return AABB(glm::min(a.min,b.min), glm::max(a.max,b.max));
        }
        static AABB Intersect(AABB a, AABB b) {
            return AABB(glm::max(a.min,b.min), glm::min(a.max,b.max));
        }

        void Corners(glm::vec4 (&corners)[8]) const {
            corners[0] = {min.x, min.y, min.z, 1.0};
            corners[1] = {max.x, min.y, min.z, 1.0};
            corners[2] = {min.x, max.y, min.z, 1.0};
            corners[3] = {max.x, max.y, min.z, 1.0};
            corners[4] = {min.x, min.y, max.z, 1.0};
            corners[5] = {max.x, min.y, max.z, 1.0};
            corners[6] = {min.x, max.y, max.z, 1.0};
            corners[7] = {max.x, max.y, max.z, 1.0};
        }
    };

    // Approximately tests whether the AABB intersects the frustum
    // defined by the input matrix
    static bool FrustumAABBTest(glm::mat4& MVP, const AABB& aabb) {
        glm::vec4 corners[8];
        aabb.Corners(corners);
        
        AABB rotated = AABB(glm::vec3(1.f),glm::vec3(-1.f));
        for (size_t corner_idx = 0; corner_idx < 8; corner_idx++) {
            glm::vec4 cornerH = MVP * corners[corner_idx];
            glm::vec3 corner = glm::vec3(cornerH) / cornerH.w;

            rotated.min = glm::min(corner,rotated.min);
            rotated.max = glm::max(corner,rotated.max);
        }

        rotated.Corners(corners);
        for (size_t corner_idx = 0; corner_idx < 8; corner_idx++) {            
            // Project vertex to clip space
            glm::vec4 corner = corners[corner_idx];
            // Check vertex against clip space bounds
            if (-1.0f < corner.x && corner.x < 1.0f &&
                -1.0f < corner.y && corner.y < 1.0 &&
                0.0f < corner.z && corner.z < 1.0)
                return true;
        }
        return false;
    }

    // As FrustumAABBTest, but the near and far plane are ignored.
    static bool FrustumAABBTestIgnoreZ(glm::mat4& MVP, const AABB& aabb) {
        glm::vec4 corners[8];
        aabb.Corners(corners);

        AABB rotated = AABB(glm::vec3(1.f),glm::vec3(-1.f));
        for (size_t corner_idx = 0; corner_idx < 8; corner_idx++) {
            glm::vec4 cornerH = MVP * corners[corner_idx];
            glm::vec3 corner = glm::vec3(cornerH) / cornerH.w;

            rotated.min = glm::min(corner,rotated.min);
            rotated.max = glm::max(corner,rotated.max);
        }

        rotated.Corners(corners);
        for (size_t corner_idx = 0; corner_idx < 8; corner_idx++) {            
            // Project vertex to clip space
            glm::vec4 corner = corners[corner_idx];
            // Check vertex against clip space bounds
            if (-1.0f < corner.x && corner.x < 1.0f &&
                -1.f < corner.y && corner.y < 1.0)
                return true;
        }
        return false;
    }
}