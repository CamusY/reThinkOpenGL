#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "Material/Material.h"
#include <nlohmann/json.hpp>

class TextureManager;

class MaterialManager : public std::enable_shared_from_this<MaterialManager> { // 继承 enable_shared_from_this
public:
    MaterialManager(std::shared_ptr<EventBus> eventBus, std::shared_ptr<TextureManager> textureManager);
    ~MaterialManager();

    std::string CreateMaterial();
    bool LoadMaterial(const nlohmann::json& materialData, std::string& errorMsg);
    nlohmann::json SaveMaterials() const;
    void DeleteMaterial(const std::string& materialUUID);
    std::shared_ptr<Material> GetMaterial(const std::string& materialUUID) const;
    std::map<std::string, std::shared_ptr<Material>> GetAllMaterials() const;
    void UpdateMaterial(const std::string& materialUUID, const MaterialData& data);
    void BindMaterial(const std::string& materialUUID) const;

    std::string LoadMaterial(const glm::vec3& diffuse, const glm::vec3& specular, float shininess, const std::string& texturePath);

private:
    std::string GenerateUUID() const;
    void SubscribeToEvents();
    void OnRequestMaterialCreation(const MyRenderer::Events::RequestMaterialCreationEvent& event);
    void OnTextureLoaded(const MyRenderer::Events::TextureLoadedEvent& event);

    std::shared_ptr<EventBus> eventBus_;
    std::shared_ptr<TextureManager> textureManager_;
    std::map<std::string, std::shared_ptr<Material>> materials_;
    mutable std::mutex mutex_;
};

#endif // MATERIAL_MANAGER_H