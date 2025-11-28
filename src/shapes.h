#pragma once

#include <glad/glad.h>
#include "mesh.h"

static Engine::Mesh* Cube() {
        /* Define vertices for a cube in 12 triangles */
    std::vector<glm::vec3> verts =
    {
        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),

        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),

        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),

        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),

        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),

        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),

        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),

        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),

        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),

        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f),

        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),

        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),
    };

    /* Manually specified normals for our cube */
    std::vector<glm::vec3> normals =
    {
        glm::vec3(0, 0, -1.f),
        glm::vec3(0, 0, -1.f),
        glm::vec3(0, 0, -1.f),
        glm::vec3(0, 0, -1.f),
        glm::vec3(0, 0, -1.f),
        glm::vec3(0, 0, -1.f),
        glm::vec3(1.f, 0, 0),
        glm::vec3(1.f, 0, 0),
        glm::vec3(1.f, 0, 0),
        glm::vec3(1.f, 0, 0),
        glm::vec3(1.f, 0, 0),
        glm::vec3(1.f, 0, 0),
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 1.f),
        glm::vec3(0, 0, 1.f),
        glm::vec3(-1.f, 0, 0),
        glm::vec3(-1.f, 0, 0),
        glm::vec3(-1.f, 0, 0),
        glm::vec3(-1.f, 0, 0),
        glm::vec3(-1.f, 0, 0),
        glm::vec3(-1.f, 0, 0),
        glm::vec3(0, -1.f, 0),
        glm::vec3(0, -1.f, 0),
        glm::vec3(0, -1.f, 0),
        glm::vec3(0, -1.f, 0),
        glm::vec3(0, -1.f, 0),
        glm::vec3(0, -1.f, 0),
        glm::vec3(0, 1.f, 0),
        glm::vec3(0, 1.f, 0),
        glm::vec3(0, 1.f, 0),
        glm::vec3(0, 1.f, 0),
        glm::vec3(0, 1.f, 0),
        glm::vec3(0, 1.f, 0),
    };

    std::vector<GLuint> indices;
    for(int i=0; i<36; i++) {
        indices.push_back(i);
    }

    return new Engine::Mesh(verts,normals,indices);
}

// Adapted from https://github.com/pmp-library/pmp-library/blob/main/src/pmp/algorithms/shapes.cpp
/* MIT License with employer disclaimer

Copyright (C) 2011-2025 the Polygon Mesh Processing Library developers.
Copyright (C) 2001-2005 by Computer Graphics Group, RWTH Aachen

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Disclaimer of Warranties and Liability. You acknowledge that the individual
providing and/or licensing this software is doing so in his or her
individual capacity, and that no past or present employer of such
individual has any role whatsoever in making this software available to
you. You expressly acknowledge that you must look solely to the individual
providing and/or licensing this software, and not to his or her past or
present employer, for any warranties, support, maintenance, upgrades,
modifications, new releases, or remedies in respect of your use of such
software. NO PAST OR PRESENT EMPLOYER OF THE INDIVIDUAL PROVIDING AND/OR
LICENSING THIS SOFTWARE MAKES ANY WARRANTY OF ANY KIND, WHETHER EXPRESS,
IMPLIED, STATUTORY OR OTHERWISE,

WITH RESPECT TO THE SOFTWARE, AND ALL SUCH WARRANTIES ARE EXPRESSLY
DISCLAIMED. IN NO EVENT WILL ANY PAST OR PRESENT EMPLOYER OF THE INDIVIDUAL
PROVIDING AND/OR LICENSING THIS SOFTWARE BE LIABLE TO YOU UNDER ANY LEGAL
OR EQUITABLE THEORY IN CONNECTION WITH THE SOFTWARE, INCLUDING FOR ANY USE,
INTERRUPTION, DELAY, OR INABILITY TO USE THE SOFTWARE, OR FOR PERSONAL
INJURY, OR ANY INCIDENTAL, SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
WHATSOEVER, INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
CORRUPTION OR LOSS OF DATA, FAILURE TO TRANSMIT OR RECEIVE ANY DATA,
BUSINESS INTERRUPTION OR ANY OTHER COMMERCIAL DAMAGES OR LOSSES, ARISING
OUT OF OR RELATED TO YOUR USE OR INABILITY TO USE THE SOFTWARE, HOWEVER
CAUSED. You agree to indemnify, defend, and hold harmless the past and
present employers of the individual providing and/or licensing this
software and their respective officers, directors, employees, agents,
affiliates, successors, and assigns from and against any and all losses,
damages, liabilities, or costs (including attorneys’ fees) resulting from
any claim, suit, action, or proceeding based on your use of the
software. The past and present employers of the individual providing and/or
licensing this software, and their respective officers, directors,
employees, agents, affiliates, successors, and assigns, are express third
party beneficiaries of this agreement with the right to enforce its terms.
*/
static Engine::Mesh* Sphere(int w, int h) {
    std::vector<glm::vec3> verts;
    std::vector<glm::vec3> normals;
    std::vector<GLuint> indices;

    float pi = glm::pi<float>();

    verts.push_back(glm::vec3(0.0,1.0,0.0));

    GLuint v0 = 0;

    for(int i=0; i<h; i++) {
        float phi = pi * float(i+1) / float(w);
        for(int j=0; j<w; j++) {
            float theta = 2.0f * pi * float(j) / float(h);
            glm::vec3 pos = glm::vec3(
                    glm::sin(phi) * glm::cos(theta),
                    glm::cos(phi),
                    glm::sin(phi) * glm::sin(theta)
                );
            verts.push_back(pos);
        }
    }

    verts.push_back(glm::vec3(0.0,-1.0,0.0));
    GLuint v1 = GLuint(verts.size() - 1);

    // add normals
    for(auto vert : verts) {
        glm::vec3 normal = glm::normalize(glm::vec3(vert));
        normals.push_back(normal);
    }

    // add top/bottom triangles
    for(int i =0; i<w; i++) {
        GLuint i0 = i+1;
        GLuint i1 = (i+1) % w + 1;
        indices.push_back(v0);
        indices.push_back(i1);
        indices.push_back(i0);

        i0 = i + w * (h-2) + 1;
        i1 = (i+1) % w + w * (h-2) + 1;
        
        indices.push_back(v1);
        indices.push_back(i0);
        indices.push_back(i1);
    }

    for(int j=0; j<h-2; j++) {
        auto j0 = j*w + 1;
        auto j1 = (j+1) * w + 1;
        for(int i=0; i<w; i++) {
            auto i0 = j0 + i;
            auto i1 = j0 + (i+1) % w;
            auto i2 = j1 + (i+1) % w;
            auto i3 = j1 + i;
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            
            indices.push_back(i0);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }

    return new Engine::Mesh(verts,normals,indices);
}