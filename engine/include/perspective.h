#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/reciprocal.hpp"

namespace Engine {
    class Perspective {
    public:
        // ref https://tomhultonharrop.com/posts/reverse-z/
        
        // Remap a classic projection to reverse Z
        static glm::mat4 reverse_z(const glm::mat4& projection) {
            constexpr glm::mat4 reverse_z {1.0f, 0.0f,  0.0f, 0.0f,
                                            0.0f, 1.0f,  0.0f, 0.0f,
                                            0.0f, 0.0f, -1.0f, 0.0f,
                                            0.0f, 0.0f,  1.0f, 1.0f};
            return reverse_z * projection;
        }

        // Remap a classic Opengl projection (depth range -1.0 to 1.0)
        // to 0.0 to 1.0 depth
        static glm::mat4 normalize_unit_range(const glm::mat4& projection)
        {
            constexpr glm::mat4 normalize_range {1.0f, 0.0f, 0.0f, 0.0f,
                                        0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 0.5f, 0.0f,
                                        0.0f, 0.0f, 0.5f, 1.0f};
            return normalize_range * projection;
        }

        // https://gist.github.com/pezcode/1609b61a1eedd207ec8c5acf6f94f53a
        static glm::mat4 infinitePerspectiveFovReverseZLH_ZO(float fov, float width, float height, float zNear) {
            const float h = glm::cot(0.5f * fov);
            const float w = h * height / width;
            glm::mat4 result = glm::zero<glm::mat4>();
            result[0][0] = w;
            result[1][1] = h;
            result[2][2] = 0.0f;
            result[2][3] = -1.0f;
            result[3][2] = zNear;
            return result;
        };
    };
}