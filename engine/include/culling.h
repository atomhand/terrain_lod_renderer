// Tom Kellett 2025
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
    };

    struct CullingResult {
    public:
        bool viewResult;
        float viewDepthMin;
        float viewDepthMax;
    };

    // Returns point as modified by the fake globe curvature
    static glm::vec3 ApplyFakedGlobeCurvature(glm::vec3 point, glm::vec3& viewPos, float farPlane) {
        float d = glm::length(point-viewPos) / farPlane;
        float curveFactor = std::max(0.f,pow(d,3.f));
        float drop = curveFactor * 3000.f;
        float curvedY = std::lerp(point.y - drop,-drop,std::min(1.f,pow(curveFactor,3.0f)));
        return glm::vec3(point.x,curvedY,point.z);
    }

    // Test whether the point defined by model is inside
    // the frustum defined by VP, accounting for the faked world curvatre effect
    static bool FakedCurvatureFrustumPointTest(const glm::mat4& model, const glm::mat4& VP, glm::vec3 viewPos, float farPlane) {
        glm::vec4 p = model * glm::vec4(0.,0.,0.,1.);
        p = glm::vec4(ApplyFakedGlobeCurvature(p,viewPos,farPlane),1.0f);
        p = VP * p;
        p /= p.w;        
        
        // Check vertex against clip space bounds
        if (-1.0f < p.x && p.x < 1.0f &&
            -1.0f < p.y && p.y < 1.0 &&
            0.0f < p.z && p.z < 1.0)
            return true;
        return false;
    }

    // Test whether the point defined by M is inside
    // the frustum defined by VP
    static bool FrustumPointTest(const glm::mat4& MVP) {
        glm::vec4 p = MVP * glm::vec4(0.,0.,0.,1.);
        p /= p.w;        
        
        // Check vertex against clip space bounds
        if (-1.0f < p.x && p.x < 1.0f &&
            -1.0f < p.y && p.y < 1.0 &&
            0.0f < p.z && p.z < 1.0)
            return true;
        return false;
    }

    // Test the OOBB created by transforming AABB by model against the frustum defined by VP.
    // also returns (which awkwardly requires View matrix to be passed as a parameter as well)
    static CullingResult FrustumAABBTest(const glm::mat4& model, const glm::mat4& V, const glm::mat4& VP, glm::vec3 viewPos, const AABB& aabb, float farPlane, bool applyFakeCurvature = false) {
        glm::vec4 corners[8];
        aabb.Corners(corners);

        CullingResult result;
        result.viewDepthMin = 99999999.f;
        result.viewDepthMax = -99999999.f;
        result.viewResult = false;
        
        AABB rotated;
        for (size_t corner_idx = 0; corner_idx < 8; corner_idx++) {
            glm::vec4 corner = model * corners[corner_idx];
            if(applyFakeCurvature) {
                corner = glm::vec4(ApplyFakedGlobeCurvature(glm::vec3(corner),viewPos,farPlane),1.0f);
            }

            glm::vec4 cornerV = V * corner;
            cornerV /= cornerV.w;
            result.viewDepthMin = std::min(-cornerV.z,result.viewDepthMin);
            result.viewDepthMax = std::max(-cornerV.z,result.viewDepthMax);

            glm::vec4 cornerP = VP * corner;
            cornerP /= cornerP.w;

            rotated.min = glm::min(glm::vec3(cornerP),rotated.min);
            rotated.max = glm::max(glm::vec3(cornerP),rotated.max);
        }

        rotated.Corners(corners);
        for (size_t corner_idx = 0; corner_idx < 8; corner_idx++) {            
            // Project vertex to clip space
            glm::vec4 corner = corners[corner_idx];
            // Check vertex against clip space bounds
            if (-1.0f < corner.x && corner.x < 1.0f &&
                -1.0f < corner.y && corner.y < 1.0 &&
                0.0f < corner.z && corner.z < 1.0) {
                    result.viewResult = true;
                    break;
                }
        }
        return result;
    }

    // Test AABB against a frustum, ignoring near and far plane
    // This is used for directional light frustums only, so curvature is not accounted for
    // There could be optimisations to take advantage of the fact that directional light frustums are always orthographic
    static bool FrustumAABBTestIgnoreZ(const glm::mat4& MVP, const AABB& aabb) {
        glm::vec4 corners[8];
        aabb.Corners(corners);

        AABB rotated;
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