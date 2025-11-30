#pragma once
#include "mesh.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <vector>
#include <filesystem>

namespace Engine {
    class AssimpWrapper {
    private:
        // from https://github.com/assimp/assimp/blob/master/samples/SimpleOpenGL/Sample_SimpleOpenGL.c
        static void RecursiveImportMesh(
            const aiScene* scene,
            const aiNode* nd,
            std::vector<Mesh> &meshes)
        {
            unsigned int i;
            unsigned int n = 0, t, v;

            // TODO - Apply the mesh transformation provided by Assimp
            //aiMatrix4x4 m = nd->mTransformation;

            /* import all meshes assigned to this node */
            for (; n < nd->mNumMeshes; ++n) {
                const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

                std::vector<glm::vec3> verts;
                std::vector<glm::vec3> normals;
                std::vector<glm::vec3> tangents;
                std::vector<glm::vec3> bitangents;
                std::vector<glm::vec2> uvs;
                std::vector<GLuint> indices;

                for (t = 0; t < mesh->mNumFaces; ++t) {
                    const aiFace* face = &mesh->mFaces[t];

                    // skip non-triangular faces
                    if(face->mNumIndices != 3) {
                        continue;
                    }

                    for(i = 0; i < face->mNumIndices; i++) {
                        int index = face->mIndices[i];
                        indices.push_back(index);
                    }
                }                

                unsigned int vertsc = (mesh->mNumVertices);
                std::cout << "Mesh has " << vertsc << " verts" << std::endl;

                for(v =0; v<mesh->mNumVertices; v++) {
                    if(mesh->mNormals != NULL) {
                        auto normal = &mesh->mNormals[v];
                        normals.push_back(glm::vec3(normal->x,normal->y,normal->z));
                    }
                    if(mesh->mTextureCoords[0]) {
                        auto uv = &mesh->mTextureCoords[0][v];
                        uvs.push_back(glm::vec2(uv->x,uv->y));
                    }
                    if(mesh->HasTangentsAndBitangents()) {
                        auto tangent = &mesh->mTangents[v];
                        auto bitangent = &mesh->mBitangents[v];
                        tangents.push_back(glm::vec3(tangent->x,tangent->y,tangent->z));
                        bitangents.push_back(glm::vec3(bitangent->x,bitangent->y,bitangent->z));
                    }

                    auto vert = &mesh->mVertices[v];
                    verts.push_back(glm::vec3(vert->x,vert->y,vert->z));

                    // OTHER vertex properties go here
                }


                std::cout << "Imported mesh with assimp, " << verts.size() << " verts, " << normals.size() << " normals, " << indices.size() << " indices" << std::endl;

                Mesh outMesh;
                outMesh.SetVerts(verts);
                if(normals.size() == verts.size())
                    outMesh.SetNormals(normals);
                if(uvs.size() == verts.size())
                    outMesh.SetUvs(uvs);
                if(tangents.size() == verts.size())
                    outMesh.SetTangentsAndBitangents(tangents,bitangents);
                outMesh.SetIndices(indices);
                outMesh.Apply();

                meshes.push_back(outMesh);
            }

            /* import all children */
            for (n = 0; n < nd->mNumChildren; ++n) {
                RecursiveImportMesh(scene, nd->mChildren[n], meshes);
            }
        }
    public:
        // Import mesh from file
        // File path is provided relative to the assets directory
        static std::vector<Mesh> ImportMesh(const char *filePath) {
            std::filesystem::path path = std::filesystem::current_path();
            path += "/assets/";
            path += filePath;

            std::cout << "importing " << path.string() << std::endl;
            const aiScene* scene = aiImportFile(path.string().c_str(),aiProcessPreset_TargetRealtime_MaxQuality);

            std::vector<Mesh> meshes;

            RecursiveImportMesh(scene, scene->mRootNode,meshes);
            
            return meshes;
        }
    };
}