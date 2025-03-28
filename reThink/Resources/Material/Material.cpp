#include "Material.h"
#include "MaterialManager/MaterialManager.h" // 假设 MaterialManager 的头文件
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

using namespace MyRenderer::Events;

Material::Material(std::shared_ptr<EventBus> eventBus, std::shared_ptr<MaterialManager> materialManager, const std::string& uuid)
    : eventBus_(eventBus), materialManager_(materialManager), uuid_(uuid) {
    if (!eventBus_) throw std::invalid_argument("Material: EventBus cannot be null");
    if (!materialManager_) throw std::invalid_argument("Material: MaterialManager cannot be null");
    if (uuid_.empty()) throw std::invalid_argument("Material: UUID cannot be empty");
    SubscribeToEvents();
    eventBus_->Publish(MaterialUpdatedEvent{uuid_, diffuseColor_, specularColor_, shininess_, textureUUID_});
}

void Material::SetDiffuseColor(const glm::vec3& color) {
    diffuseColor_ = color;
    eventBus_->Publish(MaterialUpdatedEvent{uuid_, diffuseColor_, specularColor_, shininess_, textureUUID_});
}

void Material::SetSpecularColor(const glm::vec3& color) {
    specularColor_ = color;
    eventBus_->Publish(MaterialUpdatedEvent{uuid_, diffuseColor_, specularColor_, shininess_, textureUUID_});
}

void Material::SetShininess(float shininess) {
    shininess_ = shininess;
    eventBus_->Publish(MaterialUpdatedEvent{uuid_, diffuseColor_, specularColor_, shininess_, textureUUID_});
}

void Material::SetTextureUUID(const std::string& textureUUID) {
    textureUUID_ = textureUUID;
    eventBus_->Publish(MaterialUpdatedEvent{uuid_, diffuseColor_, specularColor_, shininess_, textureUUID_});
}

void Material::BindShader(const std::string& vertexPath, const std::string& fragmentPath) {
    vertexShaderPath_ = vertexPath;
    fragmentShaderPath_ = fragmentPath;
    eventBus_->Publish(MaterialUpdatedEvent{uuid_, diffuseColor_, specularColor_, shininess_, textureUUID_});
}

void Material::Apply() const {
    if (!vertexShaderPath_.empty() && !fragmentShaderPath_.empty()) {
        glUniform3fv(glGetUniformLocation(1, "material.diffuse"), 1, glm::value_ptr(diffuseColor_));
        glUniform3fv(glGetUniformLocation(1, "material.specular"), 1, glm::value_ptr(specularColor_));
        glUniform1f(glGetUniformLocation(1, "material.shininess"), shininess_);
        // 纹理绑定由 MaterialManager 或外部渲染逻辑处理
    }
}

void Material::SubscribeToEvents() {
    eventBus_->Subscribe<MaterialUpdatedEvent>([this](const MaterialUpdatedEvent& event) {
        OnMaterialUpdated(event);
    });
}

void Material::OnMaterialUpdated(const MaterialUpdatedEvent& event) {
    if (event.materialUUID != uuid_) return;
    diffuseColor_ = event.diffuseColor;
    specularColor_ = event.specularColor;
    shininess_ = event.shininess;
    textureUUID_ = event.textureUUID;
}