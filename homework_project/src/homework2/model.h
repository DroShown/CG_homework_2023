#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include "mesh.h"
#include "kmeans/kmeans.hpp"
using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    //存储所有mesh，其中mesh[0]为外表面，mesh[1]为内表面，mesh[2]为碎片边缘
    vector<Mesh>    meshes;
    //存储了每一个碎片的中心
    vector<glm::vec3> centers;
    //存储了每一个面元的中心
    vector<glm::vec3> face_centers;
    //存储了每一个碎片包含的面元的index
    vector<vector<unsigned int>> part_face_idx;
    //存储了每个碎片边缘条带的大小（face个数）
    vector<unsigned int> edge_sizes;
    string directory;
    bool gammaCorrection;
    Assimp::Importer importer;
    const aiScene* scene;
    //存储y方向的速度大小
    float v_y;
    //存储按idx划分的batch大小
    int batch;
    glm::vec3 pos;
    glm::vec3 tar;




    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
        batch = 18;
        init_center();
        v_y = 0.0f;
    }

    Model(string const &path, string const &path_inner)
    {
        std::cout << "loading model..." << std::endl;
        loadModel(path);
        loadModel(path_inner);
        std::cout << "initing model..." << std::endl;
        meshes[0].SendPosition();
        batch = 18;
        init_center();
        v_y = 0.0f;
    }

    //载入小球，一定要在segment完成之后！
    void LoadBall(string const &path, float scale = 0.05f)
    {
        loadModel(path);
        pos = glm::vec3(-10.0f, 2.0f, 8.0f);
        tar = -0.2f * pos;
        for(auto &vert: meshes[3].vertices)
        {
            vert.Position = vert.Position * scale;
            vert.Position += pos;
        }
        meshes[3].SendPosition();
    }

    void BallMove(float time)
    {

        glm::mat4 model = glm::mat4(1.0f);
        pos += time * tar;
        for(auto &vert: meshes[3].vertices)
        {
            vert.Position += time * tar;
        }
        meshes[3].SendPosition();

    }

    bool detect_collision(float radius)
    {
        for(auto &center: face_centers)
        {
            auto dist = std::pow(pos.x - center.x,2) + std::pow(pos.y - center.y,2) + std::pow(pos.z - center.z,2);
            if(dist < radius && dist > -radius) {
                cout << "collision detected!\n position:" << pos.x << ',' << pos.y << ',' << pos.z << endl;
                crack();
                return true;
            }
        }
        return false;
    }

    //使用kmeans方法划分mesh
    void Segment_kmeans(int k)
    {
        std::cout << "segmenting model..." << std::endl;
        init_face_center();
        kmeans kmeans(k,0);
        auto center = glm::vec3(-1.57078,0.314155,1.25662);
        kmeans.initFromVec3(centers, &center);
        kmeans.assign();
        //kmeans.PrintMeans();
        //设置part face index
        part_face_idx.resize(k);
        assert(kmeans.points_.size() * 3 == this->meshes[0].vertices.size());
        for(unsigned int i = 0; i < kmeans.points_.size(); ++i)
        {
            auto cluster = kmeans.points_[i].cluster_;
            part_face_idx[cluster].push_back(i);
        }
        centers.clear();
        for(auto &part: part_face_idx)
        {
            glm::vec3 center = glm::vec3(0.0f);
            for(auto &idx: part)
            {
                center += face_centers[idx];
            }
            center = center / (float)part.size();
            centers.push_back(center);
        }
        initEdge();

    }


    // draws the model, and thus all its meshes
    void Draw(Shader &shader) {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
    //绘制指定的mesh
    void Draw_idx(Shader &shader, int idx)
    {
        meshes[idx].Draw(shader);
    }
    //按照三角面片爆炸
    void Explode(float  time)
    {
        time *= 0.5;
        auto d_y = 0.0f;
        if(v_y < 500.0f)
        {
            v_y += time * 0.5f;
            d_y = -1.0f * v_y * time;
        }


            int total = meshes[0].vertices.size() / batch;
            for(int j = 0; j < total; ++j)
            {
                //对于每个三角面片
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, 2.0f * time * centers[j]);
                model = glm::translate(model, glm::vec3(3.0f * time , d_y, 0.0f));
                for(int k = 0; k < batch; ++k)
                {
                    auto idx = batch * j + k;
                    auto new_pos_0 = model * glm::vec4(meshes[0].vertices[idx].Position,1);
                    auto new_pos_1 = model * glm::vec4 (meshes[1].vertices[idx].Position,1);
                    if(k == 0 && new_pos_0.y < -3.0f) break;
                    meshes[0].vertices[idx].Position = glm::vec3(new_pos_0);
                    meshes[1].vertices[idx].Position = glm::vec3(new_pos_1);

                }
            }
            meshes[0].SendPosition();
            meshes[1].SendPosition();
    }
    //按照kmeans划分的碎片爆炸
    void Explode_kmeans(float  time)
    {
        time *= 1.5;
        auto d_y = 0.0f;
        if(v_y < 500.0f)
        {
            v_y += time * 0.5f;
            d_y = -1.0f * v_y * time;
        }

        unsigned int edge_idx = 0;
        auto total = part_face_idx.size();
        for(int j = 0; j < total; ++j)
        {
            if(part_face_idx[j].empty()) continue;
            //if(meshes[0].vertices[3 * part_face_idx[j][0]].Position.y < -5.0f) continue;
            //对于每个碎片
            glm::mat4 model = glm::mat4(1.0f);

            model = glm::translate(model, time * (0.6f * centers[j] + 0.4f * face_centers[j+2000]));
            model = glm::translate(model, glm::vec3(3.0f * time , 0.3 * time + d_y, 0.0f));

            for(auto k: part_face_idx[j])
            {
                //对于每个三角面片
                for(int s = 0; s <3; ++s){
                    auto idx = 3*k+s;
                    auto new_pos_0 = model * glm::vec4(meshes[0].vertices[idx].Position,1);
                    auto new_pos_1 = model * glm::vec4 (meshes[1].vertices[idx].Position,1);

                    meshes[0].vertices[idx].Position = glm::vec3(new_pos_0);
                    meshes[1].vertices[idx].Position = glm::vec3(new_pos_1);
                }
            }
            for(auto s = edge_idx; s < edge_idx + edge_sizes[j]; ++s)
            {
                auto new_pos = model * glm::vec4(meshes[2].vertices[s].Position,1);
                meshes[2].vertices[s].Position = glm::vec3(new_pos);
            }
            edge_idx += edge_sizes[j];

        }
        meshes[0].SendPosition();
        meshes[1].SendPosition();
        meshes[2].SendPosition();
    }
    
private:
    bool isNeighbor(int a, int b)
    {
        auto same_cnt = 0;
        const auto &vertices = meshes[0].vertices;
        for(int i = 0; i < 3; ++i)
            for(int j = 0;j < 3; ++j)
                if(vertices[3 * a + i].Position == vertices[3 * b + j].Position) {
                    same_cnt++;
                    if(same_cnt == 2) return true;
                }
        return false;
    }

    //建立edge mesh，要求kmeans碎片划分完成
    void initEdge()
    {
        //0.创建mesh的vertices，indices
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;
        //1.对于所有碎片，循环
        for(auto part: part_face_idx)
        {
            //2.建立map: 2 edge point->2 edge vertex idx
            std::map<std::array<float, 6>,std::array<unsigned int,2>> edge_map;
            //3.遍历碎片中所有的face更新map，具体步骤为：如果map中已有edge，删除此edge;如果没有，就添加此edge
            for(auto face_idx: part)
            {
                auto point1 = meshes[0].vertices[3 * face_idx].Position;
                auto point2 = meshes[0].vertices[3 * face_idx + 1].Position;
                auto point3 = meshes[0].vertices[3 * face_idx + 2].Position;
                std::vector<std::array<float, 6>> point_array;
                point_array.push_back({point1.x,point1.y,point1.z,point2.x,point2.y,point2.z});
                point_array.push_back({point2.x,point2.y,point2.z,point1.x,point1.y,point1.z});
                point_array.push_back({point2.x,point2.y,point2.z,point3.x,point3.y,point3.z});
                point_array.push_back({point3.x,point3.y,point3.z,point2.x,point2.y,point2.z});
                point_array.push_back({point3.x,point3.y,point3.z,point1.x,point1.y,point1.z});
                point_array.push_back({point1.x,point1.y,point1.z,point3.x,point3.y,point3.z});
                for(int i = 0; i < 3; i++)
                {
                    auto &edge = point_array[2*i];
                    auto &edge_reverse = point_array[2*i+1];
                    if(edge_map.find(edge) != edge_map.end())
                    {
                        edge_map.erase(edge);

                    }
                    else if(edge_map.find(edge_reverse) != edge_map.end())
                    {
                        edge_map.erase(edge_reverse);
                    }
                    else
                    {
                        edge_map[edge] = {3 * face_idx + i, 3 * face_idx + (i+1)%3};
                    }
                }
            }
            //4.将结果存入vertices，indices，edge_sizes
            for(auto &edge: edge_map)
            {
                auto &edge_idx = edge.second;
                auto inner_vert0 = meshes[1].vertices[edge_idx[0]];
                auto inner_vert1 = meshes[1].vertices[edge_idx[1]];
                inner_vert0.Normal = -1.0f * inner_vert0.Normal;
                inner_vert1.Normal = -1.0f * inner_vert1.Normal;

                vertices.push_back(meshes[0].vertices[edge_idx[0]]);
                vertices.push_back(meshes[0].vertices[edge_idx[1]]);
                vertices.push_back(inner_vert0);
                vertices.push_back(inner_vert0);
                vertices.push_back(meshes[0].vertices[edge_idx[1]]);
                vertices.push_back(inner_vert1);

                indices.push_back(vertices.size() - 6);
                indices.push_back(vertices.size() - 5);
                indices.push_back(vertices.size() - 4);
                indices.push_back(vertices.size() - 3);
                indices.push_back(vertices.size() - 2);
                indices.push_back(vertices.size() - 1);
            }
            edge_sizes.push_back(edge_map.size() * 6);
        }

        //5.建立mesh，存入meshes
        meshes.emplace_back(vertices, indices, textures);
        //
    }

    void crack(){
        float time = 0.02f;
        auto d_y = 0.0f;
        if(v_y < 500.0f)
        {
            v_y += time * 0.5f;
            d_y = -1.0f * v_y * time;
        }

        unsigned int edge_idx = 0;
        auto total = part_face_idx.size();
        for(int j = 0; j < total; ++j)
        {
            if(part_face_idx[j].empty()) continue;
            //if(meshes[0].vertices[3 * part_face_idx[j][0]].Position.y < -15.0f) continue;
            //对于每个碎片
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, time * centers[j]);
            model = glm::translate(model, glm::vec3(3.0f * time , d_y, 0.0f));

            for(auto k: part_face_idx[j])
            {
                //对于每个三角面片
                for(int s = 0; s <3; ++s){
                    auto idx = 3*k+s;
                    auto new_pos_0 = model * glm::vec4(meshes[0].vertices[idx].Position,1);
                    auto new_pos_1 = model * glm::vec4 (meshes[1].vertices[idx].Position,1);

                    meshes[0].vertices[idx].Position = glm::vec3(new_pos_0);
                    meshes[1].vertices[idx].Position = glm::vec3(new_pos_1);
                }
            }
            for(auto s = edge_idx; s < edge_idx + edge_sizes[j]; ++s)
            {
                auto new_pos = model * glm::vec4(meshes[2].vertices[s].Position,1);
                meshes[2].vertices[s].Position = glm::vec3(new_pos);
            }
            edge_idx += edge_sizes[j];

        }
        meshes[0].SendPosition();
        meshes[1].SendPosition();
        meshes[2].SendPosition();
    }

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);

    }

    void init_face_center()
    {
        auto &mesh = meshes[0];
        auto total = mesh.vertices.size() / 3;
        for(int i = 0; i < total; ++i)
        {
            auto center = glm::vec3(0.0f, 0.0f, 0.0f);
            for(int j = 0; j < 3; ++j) center += mesh.vertices[3 * i + j].Position;

            center = center / 3.0f;
            face_centers.push_back(center);
        }
    }

    void init_center() {
        auto old_batch = batch;
        batch = 3;
        auto &mesh = meshes[0];
        auto total = mesh.vertices.size() / batch;
        for(int i = 0; i < total; ++i)
        {
            auto center = glm::vec3(0.0f, 0.0f, 0.0f);
            for(int j = 0; j < batch; ++j) center += mesh.vertices[batch * i + j].Position;

            center = center / (float )batch;
            centers.push_back(center);
        }
        batch = old_batch;
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
            }
        }
        return textures;
    }
};


unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
#endif
