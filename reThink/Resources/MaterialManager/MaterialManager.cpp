#include "MaterialManager.h"
#include "TextureManager/TextureManager.h"
#include <stdexcept>
#include <random>
#include <chrono>
#include <sstream>

MaterialManager::MaterialManager(std::shared_ptr<EventBus> eventBus, std::shared_ptr<TextureManager> textureManager)
    : eventBus_(std::move(eventBus)), textureManager_(std::move(textureManager)) {
    if (!eventBus_) throw std::invalid_argument("MaterialManager: EventBus cannot be null");
    if (!textureManager_) throw std::invalid_argument("MaterialManager: TextureManager cannot be null");
    SubscribeToEvents();
}

MaterialManager::~MaterialManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [uuid, material] : materials_) {
        if (!material->GetTextureUUID().empty()) {
            textureManager_->Release(material->GetTextureUUID());
        }
    }
    materials_.clear();

}

std::string MaterialManager::CreateMaterial() {
    std::string uuid = GenerateUUID();
    auto material = std::make_shared<Material>(eventBus_, shared_from_this(), uuid);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        materials_[uuid] = material;
    }
    eventBus_->Publish(MyRenderer::Events::MaterialCreatedEvent{uuid});
    return uuid;
}

bool MaterialManager::LoadMaterial(const nlohmann::json& materialData, std::string& errorMsg) {
    try {
        std::string uuid = materialData.at("uuid").get<std::string>();
        glm::vec3 diffuseColor = {
            materialData.at("diffuseColor")[0].get<float>(),
            materialData.at("diffuseColor")[1].get<float>(),
            materialData.at("diffuseColor")[2].get<float>()
        };
        glm::vec3 specularColor = {
            materialData.at("specularColor")[0].get<float>(),
            materialData.at("specularColor")[1].get<float>(),
            materialData.at("specularColor")[2].get<float>()
        };
        float shininess = materialData.at("shininess").get<float>();
        std::string textureUUID = materialData.value("textureUUID", "");
        std::string vertexShaderPath = materialData.value("vertexShaderPath", "");
        std::string fragmentShaderPath = materialData.value("fragmentShaderPath", "");

        auto material = std::make_shared<Material>(eventBus_, shared_from_this(), uuid);
        material->SetDiffuseColor(diffuseColor);
        material->SetSpecularColor(specularColor);
        material->SetShininess(shininess);
        material->SetTextureUUID(textureUUID);
        material->BindShader(vertexShaderPath, fragmentShaderPath);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            materials_[uuid] = material;
        }
        eventBus_->Publish(MyRenderer::Events::MaterialCreatedEvent{uuid});
        return true;
    } catch (const std::exception& e) {
        errorMsg = "Failed to load material: " + std::string(e.what());
        return false;
    }
}

nlohmann::json MaterialManager::SaveMaterials() const {
    nlohmann::json jsonMaterials = nlohmann::json::array();
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [uuid, material] : materials_) {
        nlohmann::json matData;
        matData["uuid"] = uuid;
        matData["diffuseColor"] = {material->GetDiffuseColor().x, material->GetDiffuseColor().y, material->GetDiffuseColor().z};
        matData["specularColor"] = {material->GetSpecularColor().x, material->GetSpecularColor().y, material->GetSpecularColor().z};
        matData["shininess"] = material->GetShininess();
        matData["textureUUID"] = material->GetTextureUUID();
        matData["vertexShaderPath"] = material->GetVertexShaderPath();
        matData["fragmentShaderPath"] = material->GetFragmentShaderPath();
        jsonMaterials.push_back(matData);
    }
    return jsonMaterials;
}

void MaterialManager::DeleteMaterial(const std::string& materialUUID) {
    if (materialUUID.empty()) throw std::invalid_argument("MaterialManager: Material UUID cannot be empty");
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = materials_.find(materialUUID);
    if (it != materials_.end()) {
        auto material = it->second;
        MaterialData oldData{
            materialUUID,
            material->GetDiffuseColor(),
            material->GetSpecularColor(),
            material->GetShininess(),
            material->GetTextureUUID(),
            material->GetVertexShaderPath(),
            material->GetFragmentShaderPath()
        };
        Operation op{
            [this, materialUUID]() { DeleteMaterial(materialUUID); },
            [this, oldData]() { UpdateMaterial(oldData.uuid, oldData); }
        };
        eventBus_->Publish(MyRenderer::Events::PushUndoOperationEvent{op});
        // 释放纹理引用
        if (!material->GetTextureUUID().empty()) {
            textureManager_->Release(material->GetTextureUUID());
        }
        eventBus_->Publish(MyRenderer::Events::MaterialDeletedEvent{materialUUID});
        materials_.erase(it);
    }
}

std::shared_ptr<Material> MaterialManager::GetMaterial(const std::string& materialUUID) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = materials_.find(materialUUID);
    return (it != materials_.end()) ? it->second : nullptr;
}

std::map<std::string, std::shared_ptr<Material>> MaterialManager::GetAllMaterials() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return materials_;
}

void MaterialManager::UpdateMaterial(const std::string& materialUUID, const MaterialData& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = materials_.find(materialUUID);
    if (it != materials_.end()) {
        auto material = it->second;
        MaterialData oldData{
            materialUUID,
            material->GetDiffuseColor(),
            material->GetSpecularColor(),
            material->GetShininess(),
            material->GetTextureUUID(),
            material->GetVertexShaderPath(),
            material->GetFragmentShaderPath()
        };
        // 更新纹理引用计数
        if (oldData.textureUUID != data.textureUUID) {
            if (!oldData.textureUUID.empty()) {
                textureManager_->Release(oldData.textureUUID);
            }
            if (!data.textureUUID.empty()) {
                textureManager_->AddRef(data.textureUUID);
            }
        }
        Operation op{
            [this, materialUUID, data]() { UpdateMaterial(materialUUID, data); },
            [this, materialUUID, oldData]() { UpdateMaterial(materialUUID, oldData); }
        };
        eventBus_->Publish(MyRenderer::Events::PushUndoOperationEvent{op});
        material->SetDiffuseColor(data.diffuseColor);
        material->SetSpecularColor(data.specularColor);
        material->SetShininess(data.shininess);
        material->SetTextureUUID(data.textureUUID);
        material->BindShader(data.vertexShaderPath, data.fragmentShaderPath);
        eventBus_->Publish(MyRenderer::Events::MaterialUpdatedEvent{
            materialUUID, data.diffuseColor, data.specularColor, data.shininess, data.textureUUID
        });
    }
}

void MaterialManager::BindMaterial(const std::string& materialUUID) const {
    auto material = GetMaterial(materialUUID);
    if (material) {
        material->Apply();
    }
}

std::string MaterialManager::LoadMaterial(const glm::vec3& diffuse, const glm::vec3& specular, float shininess, const std::string& texturePath) {
    std::string uuid = CreateMaterial();
    auto material = GetMaterial(uuid);
    material->SetDiffuseColor(diffuse);
    material->SetSpecularColor(specular);
    material->SetShininess(shininess);
    if (!texturePath.empty()) {
        std::string textureUUID = textureManager_->LoadTexture(texturePath);
        material->SetTextureUUID(textureUUID);
    }
    // 设置默认着色器路径
    material->BindShader("Shaders/default.vs", "Shaders/default.fs");
    eventBus_->Publish(MyRenderer::Events::MaterialUpdatedEvent{
        uuid, diffuse, specular, shininess, material->GetTextureUUID()
    });
    return uuid;
}


std::string MaterialManager::GenerateUUID() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << std::hex << now << dis(gen) << dis2(gen);
    return ss.str();
}

void MaterialManager::SubscribeToEvents() {
    eventBus_->Subscribe<MyRenderer::Events::RequestMaterialCreationEvent>(
        [this](const MyRenderer::Events::RequestMaterialCreationEvent& event) {
            OnRequestMaterialCreation(event);
        });
    eventBus_->Subscribe<MyRenderer::Events::TextureLoadedEvent>(
        [this](const MyRenderer::Events::TextureLoadedEvent& event) {
            OnTextureLoaded(event);
        });
}

void MaterialManager::OnRequestMaterialCreation(const MyRenderer::Events::RequestMaterialCreationEvent&) {
    CreateMaterial();
}

void MaterialManager::OnTextureLoaded(const MyRenderer::Events::TextureLoadedEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [uuid, material] : materials_) {
        if (material->GetTextureUUID() == event.uuid) {
            if (event.success) {
                eventBus_->Publish(MyRenderer::Events::MaterialUpdatedEvent{
                    uuid,
                    material->GetDiffuseColor(),
                    material->GetSpecularColor(),
                    material->GetShininess(),
                    event.uuid
                });
            } else {
                material->SetTextureUUID("");
                eventBus_->Publish(MyRenderer::Events::MaterialUpdatedEvent{
                    uuid,
                    material->GetDiffuseColor(),
                    material->GetSpecularColor(),
                    material->GetShininess(),
                    ""
                });
            }
        }
    }
}