// Material.cpp
#include "Material.h"
#include <glad/glad.h>

Material::Material(std::shared_ptr<ShaderManager> shaderManager)
    : shaderManager_(shaderManager) {
    if (!shaderManager_) {
        throw std::invalid_argument("Material: ShaderManager 不存在");
    }
}

//------------------- 属性设置方法实现 -------------------//
void Material::SetDiffuseColor(const glm::vec3& color) {
    diffuseColor_ = color;
}

void Material::SetSpecularColor(const glm::vec3& color) {
    specularColor_ = color;
}

void Material::SetDiffuseTexture(GLuint textureID) {
    diffuseTextureID_ = textureID;
}

void Material::SetNormalTexture(GLuint textureID) {
    normalTextureID_ = textureID;
}

void Material::SetRoughness(float roughness) {
    roughness_ = roughness < 0.0f ? 0.0f : (roughness > 1.0f ? 1.0f : roughness);
}

void Material::SetMetallic(float metallic) {
    metallic_ = metallic < 0.0f ? 0.0f : (metallic > 1.0f ? 1.0f : metallic);
}

//------------------- 属性获取方法实现 -------------------//
glm::vec3 Material::GetDiffuseColor() const {
    return diffuseColor_;
}

glm::vec3 Material::GetSpecularColor() const {
    return specularColor_;
}

GLuint Material::GetDiffuseTexture() const {
    return diffuseTextureID_;
}

GLuint Material::GetNormalTexture() const {
    return normalTextureID_;
}

float Material::GetRoughness() const {
    return roughness_;
}

float Material::GetMetallic() const {
    return metallic_;
}

//------------------- 核心绑定逻辑 -------------------//
void Material::Bind(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
    // 获取着色器程序ID
    GLuint programID = shaderManager_->GetShaderProgram(vertexShaderPath, fragmentShaderPath);
    if (programID == 0) {
        throw std::runtime_error("Material::Bind: 着色器未加载 - " +
                                 vertexShaderPath + " + " + fragmentShaderPath);
    }

    // 应用Uniform设置
    ApplyUniforms(programID);
}

void Material::ApplyUniforms(GLuint programID) const {
    glUseProgram(programID);

    // 设置颜色Uniform
    GLint loc = glGetUniformLocation(programID, "u_DiffuseColor");
    if (loc != -1) glUniform3fv(loc, 1, &diffuseColor_[0]);

    loc = glGetUniformLocation(programID, "u_SpecularColor");
    if (loc != -1) glUniform3fv(loc, 1, &specularColor_[0]);

    // 设置纹理Uniform
    if (diffuseTextureID_ != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTextureID_);
        loc = glGetUniformLocation(programID, "u_DiffuseTexture");
        if (loc != -1) glUniform1i(loc, 0);
    }

    if (normalTextureID_ != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTextureID_);
        loc = glGetUniformLocation(programID, "u_NormalTexture");
        if (loc != -1) glUniform1i(loc, 1);
    }

    // 设置物理材质属性
    loc = glGetUniformLocation(programID, "u_Roughness");
    if (loc != -1) glUniform1f(loc, roughness_);

    loc = glGetUniformLocation(programID, "u_Metallic");
    if (loc != -1) glUniform1f(loc, metallic_);
}