#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>       // OpenGL加载库
#include <assimp/scene.h>    // Assimp场景数据结构

class Model {
public:
    std::vector<float> vertices;
    unsigned int VAO = 0, VBO = 0;
    glm::vec3 position = {0, 0, 0};
    glm::vec3 rotation = {0, 0, 0};
    glm::vec3 scale = {1, 1, 1};
    bool loaded = false;

    void load(const char* path);
    void draw() const;
    glm::mat4 getModelMatrix() const;
    bool isLoaded() const { return loaded; }
};
