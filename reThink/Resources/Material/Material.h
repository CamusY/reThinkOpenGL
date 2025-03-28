#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <glm/glm.hpp>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
class MaterialManager; // 前向声明

class Material {
public:
    Material(std::shared_ptr<EventBus> eventBus, std::shared_ptr<MaterialManager> materialManager, const std::string& uuid);
    std::string GetUUID() const { return uuid_; }

    void SetDiffuseColor(const glm::vec3& color);
    glm::vec3 GetDiffuseColor() const { return diffuseColor_; }
    void SetSpecularColor(const glm::vec3& color);
    glm::vec3 GetSpecularColor() const { return specularColor_; }
    void SetShininess(float shininess);
    float GetShininess() const { return shininess_; }
    void SetTextureUUID(const std::string& textureUUID);
    std::string GetTextureUUID() const { return textureUUID_; }
    void BindShader(const std::string& vertexPath, const std::string& fragmentPath);
    std::string GetVertexShaderPath() const { return vertexShaderPath_; }
    std::string GetFragmentShaderPath() const { return fragmentShaderPath_; }
    void Apply() const;

private:
    void SubscribeToEvents();
    void OnMaterialUpdated(const MyRenderer::Events::MaterialUpdatedEvent& event);

    std::shared_ptr<EventBus> eventBus_;
    std::shared_ptr<MaterialManager> materialManager_; // 新增依赖
    std::string uuid_;
    glm::vec3 diffuseColor_ = glm::vec3(1.0f);
    glm::vec3 specularColor_ = glm::vec3(1.0f);
    float shininess_ = 32.0f;
    std::string textureUUID_;
    std::string vertexShaderPath_;
    std::string fragmentShaderPath_;
};

#endif // MATERIAL_H