#pragma once
#include <algorithm>
#include <vector>
#include <limits>
#include "glm/glm.hpp"

namespace Engine {
    // Axis aligned bounding box
    struct AABB {
    public:
        glm::vec3 min;
        glm::vec3 max;

        AABB(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}

        AABB() {
            // originally tried using std::numeric_limits but it did not work
            // as expected
            min = glm::vec3(99999999.f);
            max = glm::vec3(-99999999.f);
        }

        // Returns the AABB encompassing the provided points
        AABB(std::vector<glm::vec3>& points) {
            min = glm::vec3(99999999.f);
            max = glm::vec3(-99999999.f);

            for(auto point : points)  {
                min = glm::min(min,point);
                max = glm::max(max,point);
            }
        }

        /*
        // Returns false if the AABB is improperly initialized
        bool IsValid() const {
            return max.x >= min.x && max.y >= min.y && max.z >= min.z;
        }

        // Writes the AABs corners into the provided array
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
        */
    };
}