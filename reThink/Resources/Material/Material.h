// Material.h
#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <stdexcept>
#include "ShaderManager/ShaderManager.h"

/**
 * @brief 材质类，负责管理材质属性并与着色器进行绑定
 * @note 通过依赖注入获取 ShaderManager 实例，严格遵循生产级安全标准
 */
class Material {
public:
    /**
     * @brief 构造函数注入 ShaderManager 依赖
     * @param shaderManager ShaderManager 智能指针（必须非空）
     * @throws std::invalid_argument 参数为空时抛出
     */
    explicit Material(std::shared_ptr<ShaderManager> shaderManager);

    //------------------- 材质属性设置接口 -------------------//
    void SetDiffuseColor(const glm::vec3& color);
    void SetSpecularColor(const glm::vec3& color);
    void SetDiffuseTexture(GLuint textureID);
    void SetNormalTexture(GLuint textureID);
    void SetRoughness(float roughness);
    void SetMetallic(float metallic);

    //------------------- 材质属性获取接口 -------------------//
    glm::vec3 GetDiffuseColor() const;
    glm::vec3 GetSpecularColor() const;
    GLuint GetDiffuseTexture() const;
    GLuint GetNormalTexture() const;
    float GetRoughness() const;
    float GetMetallic() const;

    /**
     * @brief 绑定材质到指定着色器
     * @param vertexShaderPath 顶点着色器路径
     * @param fragmentShaderPath 片段着色器路径
     * @throws std::runtime_error 着色器未加载时抛出
     */
    void Bind(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);

private:
    std::shared_ptr<ShaderManager> shaderManager_;

    // 材质属性
    glm::vec3 diffuseColor_ = glm::vec3(1.0f); // 默认漫反射颜色为白色
    glm::vec3 specularColor_ = glm::vec3(0.5f); // 默认镜面反射颜色
    GLuint diffuseTextureID_ = 0;               // 漫反射纹理ID（0表示未设置）
    GLuint normalTextureID_ = 0;                // 法线纹理ID
    float roughness_ = 0.5f;                    // 粗糙度（0-1）
    float metallic_ = 0.0f;                     // 金属度（0-1）

    /**
     * @brief 设置着色器Uniform值
     * @param programID 目标着色器程序ID
     */
    void ApplyUniforms(GLuint programID) const;
};