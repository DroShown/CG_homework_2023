//
// Created by 徐琢雄 on 2023/11/25.
//

#ifndef LEARNOPENGL_GEN_INNER_H
#define LEARNOPENGL_GEN_INNER_H
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <learnopengl/filesystem.h>
#include "model.h"

void generate_inner_face(string path, float thickness)
{
    Assimp::Exporter exporter;
    Assimp::Importer importer;
    auto scene = importer.ReadFile(FileSystem::getPath(path), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    auto numMeshes = scene->mNumMeshes;
    auto meshes = *(scene->mMeshes);

    auto numVertice = meshes->mNumVertices;
    auto numFaces = meshes->mNumFaces;
    auto vertices = meshes->mVertices;
    auto normals = meshes->mNormals;
    auto faces = meshes->mFaces;

    for(int i = 0; i < numVertice; ++i) {
        vertices[i] -= thickness * normals[i];
        normals[i] = -normals[i];
    }


    auto res = exporter.Export(scene, "obj", FileSystem::getPath("resources/objects/models/stanford_bunny_inner.obj"));

}



#endif //LEARNOPENGL_GEN_INNER_H
