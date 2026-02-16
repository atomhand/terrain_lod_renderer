#ifndef SEPARATING_AXIS_GLSL
#define SEPARATING_AXIS_GLSL

struct AABB
{
    vec3 m_Min;
    vec3 m_Max;
};

struct OBB
{
    vec3 center;
    vec3 extents;
    vec3 axes[3];
};

vec2 ProjectOBB(OBB obb, vec3 M) {
    float C = dot(M, obb.center);
    float radius = 0.f;
    for(int i=0; i<3; i++) {
        radius += abs(dot(M, obb.axes[i])) * obb.extents[i];
    }
    return vec2(C - radius,C + radius);
}

// Projection of an OBB that is missing a near Z face
// and has sides that are infinitely long in the Z direction
vec2 ProjectLightFrustum(OBB obb, vec3 M) {
    float C = dot(M, obb.center);
    float radius = 0.f;
    for(int i=0; i<2; i++) {
        radius += abs(dot(M, obb.axes[i])) * obb.extents[i];
    }

    // special handling for Z

    float zProj = dot(M,obb.axes[2]);
    radius += abs(zProj) * obb.extents[2];
    vec2 projectedExtent = vec2(C-radius,C+radius);

    if(zProj < -1e-6) {
        projectedExtent.x = -1e9;
    }
    if(zProj > 1e-6) {
        projectedExtent.y = 1e9;
    }

    return projectedExtent;
}

bool IntersectFrustumOBB_SAT(const OBB a, const OBB lightFrustum) {
    // a's axes
    for(int m=0; m<3; m++) {
        const vec3 M = a.axes[m];

        vec2 aMinMax = ProjectOBB(a,M);
        vec2 bMinMax = ProjectLightFrustum(lightFrustum,M);
        if(aMinMax.x > bMinMax.y || aMinMax.y < bMinMax.x) {
            return false;
        }
    }
    // b's axes
    for(int m=0; m<3; m++) {
        const vec3 M = lightFrustum.axes[m];

        vec2 aMinMax = ProjectOBB(a,M);
        vec2 bMinMax = ProjectLightFrustum(lightFrustum,M);
        if(aMinMax.x > bMinMax.y || aMinMax.y < bMinMax.x) {
            return false;
        }
    }

    // edge cross products
    for(int edge_a=0; edge_a<3; edge_a++) {
        for(int edge_b=0; edge_b<3; edge_b++) {
            const vec3 M = cross(a.axes[edge_a],lightFrustum.axes[edge_b]);
            if(dot(M,M) < 1e-6) continue; // Skip parallel/degenerate axes

            vec2 aMinMax = ProjectOBB(a,M);
            vec2 bMinMax = ProjectLightFrustum(lightFrustum,M);
            if(aMinMax.x > bMinMax.y || aMinMax.y < bMinMax.x) {
                return false;
            }
        }
    }

    return true;
}

bool SAT_Visibility_Ortho(const mat4 model, const AABB aabb, const mat4 inView, const OBB ortho) {
    mat4 viewModel = inView * model;

    vec3 corners[4] = {
        vec3(aabb.m_Min.x,aabb.m_Min.y,aabb.m_Min.z),
        vec3(aabb.m_Max.x,aabb.m_Min.y,aabb.m_Min.z),
        vec3(aabb.m_Min.x,aabb.m_Max.y,aabb.m_Min.z),
        vec3(aabb.m_Min.x,aabb.m_Min.y,aabb.m_Max.z)
    };

    for(int i =0; i<4; i++) {
        corners[i] = (viewModel * vec4(corners[i],1.f)).xyz;
    }

    OBB obb;
    obb.axes[0] = corners[1] - corners[0];
    obb.axes[1] = corners[2] - corners[0];
    obb.axes[2] = corners[3] - corners[0];
    obb.center = corners[0] + 0.5f * (obb.axes[0] + obb.axes[1] + obb.axes[2]);
    obb.extents = vec3(length(obb.axes[0]),length(obb.axes[1]),length(obb.axes[2]));
    obb.axes[0] /= obb.extents.x;
    obb.axes[1] /= obb.extents.y;
    obb.axes[2] /= obb.extents.z;
    obb.extents *= 0.5f;

    return IntersectFrustumOBB_SAT(obb,ortho);
}

bool SAT_visibility(const mat4 model, const AABB aabb, const mat4 inView, const vec4 inFrustum) {
    const float z_near = inFrustum.z;
    const float z_far = inFrustum.w;
    const float x_near  = inFrustum.x;
    const float y_near =  inFrustum.y;

    mat4 viewModel = inView * model;

    vec3 corners[4] = {
        vec3(aabb.m_Min.x,aabb.m_Min.y,aabb.m_Min.z),
        vec3(aabb.m_Max.x,aabb.m_Min.y,aabb.m_Min.z),
        vec3(aabb.m_Min.x,aabb.m_Max.y,aabb.m_Min.z),
        vec3(aabb.m_Min.x,aabb.m_Min.y,aabb.m_Max.z)
    };

    for(int i =0; i<4; i++) {
        corners[i] = (viewModel * vec4(corners[i],1.f)).xyz;
    }

    OBB obb;
    obb.axes[0] = corners[1] - corners[0];
    obb.axes[1] = corners[2] - corners[0];
    obb.axes[2] = corners[3] - corners[0];
    obb.center = corners[0] + 0.5f * (obb.axes[0] + obb.axes[1] + obb.axes[2]);
    obb.extents = vec3(length(obb.axes[0]),length(obb.axes[1]),length(obb.axes[2]));
    obb.axes[0] /= obb.extents.x;
    obb.axes[1] /= obb.extents.y;
    obb.axes[2] /= obb.extents.z;
    obb.extents *= 0.5f;

    // frustum forward axis
    {
        const vec3 M = vec3(0.f,0.f,1.f);
        
        float MoC = obb.center.z;
        float radius = 0.0f;
        for(int i =0; i<3; i++) {
            radius += abs(obb.axes[i].z) * obb.extents[i];
        }
        float obb_min = MoC - radius;
        float obb_max = MoC + radius;

        float tau_0 = z_far; // Since z is negative, far is smaller than near
        float tau_1 = z_near;

        if(obb_min > tau_1 || obb_max < tau_0) {
            return false;
        }
    }

    // frustum side plane axes
    {
        const vec3 M[4] = {
            vec3(z_near,0.f,x_near), // Left Plane
            vec3(-z_near,0.f,x_near), // Right Plane
            vec3(0.f,-z_near,y_near), // Top Plane
            vec3(0.f,z_near,y_near) // Bottom Plane
        };
        for(int m=0; m<4; m++) {
            vec3 Mo = vec3(abs(M[m].x),abs(M[m].y),M[m].z);
            float MoC = dot(M[m], obb.center);

            float obb_radius = 0.f;
            for(int i =0; i<3; i++) {
                obb_radius += abs(dot(M[m], obb.axes[i])) * obb.extents[i];
            }
            float obb_min = MoC - obb_radius;
            float obb_max = MoC + obb_radius;

            float p = x_near * Mo.x + y_near * Mo.y;

            float tau_0 = z_near * Mo.z - p;
            float tau_1 = z_near * Mo.z + p;

            if(tau_0 < 0.f) {
                tau_0 *= z_far / z_near;
            }
            if(tau_1 > 0.f) {
                tau_1 *= z_far / z_near;
            }

            if(obb_min > tau_1 || obb_max < tau_0) {
                return false;
            }
        }
    }

    // OBB Axes
    {
        for(int m=0; m<3; m++) {
            const vec3 M = obb.axes[m];
            vec3 Mo = vec3(abs(M.x),abs(M.y),M.z);
            float MoC = dot(M, obb.center);

            float obb_radius = obb.extents[m];

            float obb_min = MoC - obb_radius;
            float obb_max = MoC + obb_radius;

            // Frustum projection
            float p = x_near * Mo.x + y_near * Mo.y;
            float tau_0 = z_near * Mo.z - p;
            float tau_1 = z_near * Mo.z + p;
            if(tau_0 < 0.f) {
                tau_0 *= z_far / z_near;
            }
            if(tau_1 > 0.f) {
                tau_1 *= z_far / z_near;
            }

            if(obb_min > tau_1 || obb_max < tau_0) {
                return false;
            }
        }
    }

    // R x A_i
    {
        for(int m=0; m<3; m++) {
            const vec3 M = vec3(0.f,-obb.axes[m].z, obb.axes[m].y);
            vec3 Mo = vec3(0.f,abs(M.y), M.z);
            float MoC = M.y * obb.center.y + M.z * obb.center.z;

            float obb_radius = 0.0f;
            for(int i =0; i<3; i++) {
                obb_radius += abs(dot(M, obb.axes[i])) * obb.extents[i];
            }
            
            float obb_min = MoC - obb_radius;
            float obb_max = MoC + obb_radius;

            float p = x_near * Mo.x + y_near * Mo.y;
            float tau_0 = z_near * Mo.z - p;
            float tau_1 = z_near * Mo.z + p;
            if(tau_0 < 0.f) {
                tau_0 *= z_far / z_near;
            }
            if(tau_1 > 0.f) {
                tau_1 *= z_far / z_near;
            }

            if(obb_min > tau_1 || obb_max < tau_0) {
                return false;
            }
        }
    }

    // U x A_i
    {
        for(int m=0; m<3; m++) {
            const vec3 M = vec3(obb.axes[m].z, 0.f, -obb.axes[m].x);
            vec3 Mo = vec3(abs(M.x), 0.f, M.z);
            float MoC = M.x * obb.center.x + M.z * obb.center.z;

            float obb_radius = 0.0f;
            for(int i =0; i<3; i++) {
                obb_radius += abs(dot(M, obb.axes[i])) * obb.extents[i];
            }
            
            float obb_min = MoC - obb_radius;
            float obb_max = MoC + obb_radius;

            float p = x_near * Mo.x + y_near * Mo.y;
            float tau_0 = z_near * Mo.z - p;
            float tau_1 = z_near * Mo.z + p;
            if(tau_0 < 0.f) {
                tau_0 *= z_far / z_near;
            }
            if(tau_1 > 0.f) {
                tau_1 *= z_far / z_near;
            }

            if(obb_min > tau_1 || obb_max < tau_0) {
                return false;
            }
        }
    }

    // Frustum edges x Ai
    for(int obb_edge_idx = 0; obb_edge_idx < 3; obb_edge_idx++) {
        const vec3 M[4] = {
            cross(vec3(-x_near,0.f,z_near), obb.axes[obb_edge_idx]), // Left Plane
            cross(vec3(x_near,0.f,z_near), obb.axes[obb_edge_idx]), // Right Plane
            cross(vec3(0.f,y_near,z_near), obb.axes[obb_edge_idx]), // Top Plane
            cross(vec3(0.f,-y_near,z_near), obb.axes[obb_edge_idx]), // Bottom Plane
        };

        for(int m=0; m<4; m++) {
            vec3 Mo = vec3(abs(M[m].x),abs(M[m].y), M[m].z);

            const float epsilon = 1e-4;
            if(Mo.x < epsilon && Mo.y < epsilon && abs(Mo.z) < epsilon) continue;
            
            float MoC = dot(M[m], obb.center);

            float obb_radius = 0.f;
            for(int i=0; i<3; i++) {
                obb_radius += abs(dot(M[m], obb.axes[i])) * obb.extents[i];
            }

            float obb_min = MoC - obb_radius;
            float obb_max = MoC + obb_radius;

            // Frustum projection
            float p = x_near * Mo.x + y_near * Mo.y;
            float tau_0 = z_near * Mo.z - p;
            float tau_1 = z_near * Mo.z + p;
            if(tau_0 < 0.f) {
                tau_0 *= z_far / z_near;
            }
            if(tau_1 > 0.f) {
                tau_1 *= z_far / z_near;
            }

            if(obb_min > tau_1 || obb_max < tau_0) {
                return false;
            }
        }
    }

    // No intersections
    return true;
}

#endif