#include "model.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

void Model::load(const char* path) {
    vertices.clear();

    // 加载模型
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate | aiProcess_FlipUVs
    );

    // 错误检查
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Assimp加载错误: " + std::string(importer.GetErrorString()));
    }

    // 释放旧的OpenGL资源
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        VAO = VBO = 0;
    }

    // 提取第一个网格的顶点数据
    aiMesh* mesh = scene->mMeshes[0];
    for (unsigned i = 0; i < mesh->mNumVertices; i++) {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
        // 可扩展：法线、UV等数据
    }

    // 生成OpenGL缓冲区
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    loaded = true;
}

void Model::draw() const {
    if (!loaded) return;
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(vertices.size()) / 3);
}

glm::mat4 Model::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);
    return model;
}
