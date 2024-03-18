// ReSharper disable CppClangTidyBugproneNarrowingConversions
#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "stb_image.h"

#include "Mesh.h"
#include "Shader.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model {
public:
  // constructor, expects a filepath to a 3D model.
  explicit Model(const string &model_name,string const &path, const bool gamma = false)
    : gamma_correction(gamma), position(0, 0, 0), rotation(1, 1, 1), scale(1) {
    LoadModel(path);
    name = model_name;
  }

  // Draws the model, and thus all its meshes
  void Draw(Shader &shader) {
    auto model_matrix = glm::mat4(1.0f);

    // Apply local space transformations
    model_matrix = glm::translate(model_matrix, position);
    model_matrix = glm::rotate(model_matrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model_matrix = glm::rotate(model_matrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model_matrix = glm::rotate(model_matrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    model_matrix = glm::scale(model_matrix, glm::vec3(scale, scale, scale));

    shader.setMat4("model", model_matrix);

    for (auto &mesh : meshes) mesh.Draw(shader);
  }

  string GetName() const{return name;}

  void SetPosition(const glm::vec3 &pos) { position = pos; }

  void SetRotation(const glm::vec3 &rot) { rotation = rot; }

  void SetScale(const float sc) { scale = sc; }

  glm::vec3 GetPosition() const { return position; }

  glm::vec3 GetRotation() const { return rotation; }

  glm::mat4 GetWorldTransform() const {
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    transform = glm::scale(transform, glm::vec3(scale, scale, scale));
    return transform;
  }

  glm::vec3 GetWorldPosition() const {
    return glm::vec3(GetWorldTransform()[3]);
  }

  glm::vec3 GetWorldRotation() const {
    glm::mat4 transform = GetWorldTransform();

    // 分解变换矩阵以获取旋转分量
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(transform, scale, rotation, translation, skew, perspective);

    // 将四元数转换为欧拉角
    return glm::degrees(glm::eulerAngles(rotation));
  }

  // model data
  vector<Texture> textures_loaded;// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
  vector<Mesh> meshes;
  string directory;
  bool gamma_correction;

  // Transform variables in local space
  glm::vec3 position;
  glm::vec3 rotation;
  float scale;

  string name;

private:
  // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
  void LoadModel(string const &path) {
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)// if is Not Zero
    {
      cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
      return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    ProcessNode(scene->mRootNode, scene);
  }

  // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
  void ProcessNode(const aiNode *node, const aiScene *scene) {
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      // the node object only contains indices to index the actual objects in the scene.
      // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.push_back(ProcessMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) { ProcessNode(node->mChildren[i], scene); }
  }

  Mesh ProcessMesh(aiMesh *mesh, const aiScene *scene) {
    // data to fill
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
      Vertex vertex;
      glm::vec3 vector;// we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
      // positions
      vector.x = mesh->mVertices[i].x;
      vector.y = mesh->mVertices[i].y;
      vector.z = mesh->mVertices[i].z;
      vertex.Position = vector;
      // normals
      if (mesh->HasNormals()) {
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;
      }
      // texture coordinates
      if (mesh->mTextureCoords[0])// does the mesh contain texture coordinates?
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
        // bi tangent
        vector.x = mesh->mBitangents[i].x;
        vector.y = mesh->mBitangents[i].y;
        vector.z = mesh->mBitangents[i].z;
        vertex.Bitangent = vector;
      } else vertex.TexCoords = glm::vec2(0.0f, 0.0f);

      vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
      aiFace face = mesh->mFaces[i];
      // retrieve all indices of the face and store them in the indices vector
      for (unsigned int j = 0; j < face.mNumIndices; j++) indices.push_back(face.mIndices[j]);
    }
    // process materials
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    vector<Texture> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    vector<Texture> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<Texture> normalMaps = LoadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps = LoadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // return a mesh object created from the extracted mesh data
    return Mesh(vertices, indices, textures);
  }

  // checks all material textures of a given type and loads the textures if they're not loaded yet.
  // the required info is returned as a Texture struct.
  vector<Texture> LoadMaterialTextures(const aiMaterial *mat, const aiTextureType type, const string &typeName) {
    vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
      aiString str;
      mat->GetTexture(type, i, &str);
      // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
      bool skip = false;
      for (unsigned int j = 0; j < textures_loaded.size(); j++) {
        if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
          textures.push_back(textures_loaded[j]);
          skip = true;// a texture with the same filepath has already been loaded, continue to next one. (optimization)
          break;
        }
      }
      if (!skip) {
        // if texture hasn't been loaded already, load it
        Texture texture;
        texture.id = TextureFromFile(str.C_Str(), this->directory);
        texture.type = typeName;
        texture.path = str.C_Str();
        textures.push_back(texture);
        textures_loaded.push_back(texture);// store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
      }
    }
    return textures;
  }
};

inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma) {
  auto filename = string(path);
  filename = directory + '/' + filename;

  unsigned int texture_id;
  glGenTextures(1, &texture_id);

  int width , height , nrComponents;
  unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
  if (data) {
    GLenum format = 0;
    if (nrComponents == 1) format = GL_RED;
    else if (nrComponents == 3) format = GL_RGB;
    else if (nrComponents == 4) format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    std::cout << "Texture failed to load at path: " << path << '\n';
    stbi_image_free(data);
  }

  return texture_id;
}
#endif
