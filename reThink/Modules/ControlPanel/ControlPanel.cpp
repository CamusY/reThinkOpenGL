#include "ControlPanel.h"
#include <imgui.h>
#include <imgui-backends/ImGuiFileDialog.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace MyRenderer {

ControlPanel::ControlPanel(std::shared_ptr<EventBus> eventBus,
                           std::shared_ptr<MaterialManager> materialManager,
                           std::shared_ptr<TextureManager> textureManager)
    : eventBus_(eventBus), materialManager_(materialManager), textureManager_(textureManager) {
    if (!eventBus_) throw std::invalid_argument(u8"ControlPanel: EventBus 不能为空");
    if (!materialManager_) throw std::invalid_argument(u8"ControlPanel: MaterialManager 不能为空");
    if (!textureManager_) throw std::invalid_argument(u8"ControlPanel: TextureManager 不能为空");
    SubscribeToEvents();
}

void ControlPanel::Update() {
    ImGui::Begin(u8"控制面板", nullptr, ImGuiWindowFlags_NoCollapse);
    RenderSceneSettings(); // 新增：渲染场景设置（包括光照控制）
    if (!hasSelectedModel_) {
        ImGui::Text(u8"未选中模型");
    } else {
        RenderUI();
    }
    ImGui::End();
}

void ControlPanel::SetSelectedModel(const ModelData& model) {
    currentModel_ = model;
    hasSelectedModel_ = true;
    if (!model.materialUUIDs.empty()) {
        auto material = materialManager_->GetMaterial(model.materialUUIDs[0]);
        if (material) {
            currentMaterial_ = MaterialData{
                model.materialUUIDs[0],
                material->GetDiffuseColor(),
                material->GetSpecularColor(),
                material->GetShininess(),
                material->GetTextureUUID(),
                material->GetVertexShaderPath(),
                material->GetFragmentShaderPath()
            };
            auto texture = textureManager_->GetTexture(currentMaterial_.textureUUID);
            currentTexturePath_ = texture ? texture->GetFilepath() : "";
        } else {
            currentMaterial_ = MaterialData{};
            currentTexturePath_ = "";
        }
    } else {
        currentMaterial_ = MaterialData{};
        currentTexturePath_ = "";
    }
}

void ControlPanel::SubscribeToEvents() {
    eventBus_->Subscribe<Events::ModelSelectionChangedEvent>([this](const auto& e) { OnModelSelectionChanged(e); });
    eventBus_->Subscribe<OperationModeChangedEvent>([this](const auto& e) { OnOperationModeChanged(e); });
    eventBus_->Subscribe<Events::ProjectLoadFailedEvent>([this](const auto& e) { OnProjectLoadFailed(e); });
    eventBus_->Subscribe<Events::ModelDeletedEvent>([this](const auto& e) { OnModelDeleted(e); });
    eventBus_->Subscribe<Events::MaterialUpdatedEvent>([this](const auto& e) { OnMaterialUpdated(e); });
    eventBus_->Subscribe<Events::TextureLoadedEvent>([this](const auto& e) { OnTextureLoaded(e); });

    // 新增：监听动画播放状态
    eventBus_->Subscribe<Events::AnimationPlaybackStartedEvent>(
        [this](const Events::AnimationPlaybackStartedEvent& event) {
            isAnimationPlaying_ = true;
        });
    eventBus_->Subscribe<Events::AnimationPlaybackStoppedEvent>(
        [this](const Events::AnimationPlaybackStoppedEvent& event) {
            isAnimationPlaying_ = false;
        });
}

void ControlPanel::RenderUI() {
    ImGui::Text(u8"选中模型: %s", currentModel_.uuid.c_str());
    RenderModelTransform();
    RenderMaterialProperties();
    RenderShaderInfo();
    RenderTextureSelector();
}

void ControlPanel::RenderSceneSettings() {
    // 新增：渲染场景光照设置
    if (ImGui::CollapsingHeader(u8"场景光照")) {
        static glm::vec3 lightDir = {0.0f, -1.0f, -1.0f}; // 默认光方向
        static glm::vec3 lightColor = {1.0f, 1.0f, 1.0f}; // 默认光颜色（白色）
        bool changed = false;

        // 调整光方向
        if (ImGui::DragFloat3(u8"光方向", &lightDir.x, 0.1f)) {
            changed = true;
        }
        // 调整光颜色
        if (ImGui::ColorEdit3(u8"光颜色", &lightColor.x)) {
            changed = true;
        }

        // 如果有变化，发布光照更新事件
        if (changed) {
            if (glm::length(lightDir) > 0.0f) {
                lightDir = glm::normalize(lightDir); // 确保方向向量标准化
            } else {
                lightDir = glm::vec3(0.0f, 0.0f, 1.0f); // 避免零向量
            }
            eventBus_->Publish(MyRenderer::Events::SceneLightUpdatedEvent{lightDir, lightColor});
        }
    }
}

void ControlPanel::RenderModelTransform() {
    ImGui::Separator();
    ImGui::Text(u8"变换");

    if (isAnimationPlaying_) {
        ImGui::Text(u8"动画播放期间无法编辑变换");
        return;
    }

    glm::vec3 position, scale;
    glm::quat rotation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(currentModel_.transform, scale, rotation, position, skew, perspective);

    if (ImGui::DragFloat3(u8"位置", &position[0], 0.1f)) {
        glm::mat4 oldTransform = currentModel_.transform;
        glm::mat4 newTransform = glm::translate(glm::mat4(1.0f), position) *
                                 glm::mat4_cast(rotation) *
                                 glm::scale(glm::mat4(1.0f), scale);
        currentModel_.transform = newTransform;

        Operation op;
        op.execute = [this, uuid = currentModel_.uuid, newTransform]() {
            eventBus_->Publish(Events::ModelTransformedEvent{uuid, newTransform});
        };
        op.undo = [this, uuid = currentModel_.uuid, oldTransform]() {
            eventBus_->Publish(Events::ModelTransformedEvent{uuid, oldTransform});
        };
        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        eventBus_->Publish(Events::ModelTransformedEvent{currentModel_.uuid, newTransform});
    }

    glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(rotation));
    if (ImGui::DragFloat3(u8"旋转", &eulerAngles[0], 1.0f)) {
        glm::mat4 oldTransform = currentModel_.transform;
        rotation = glm::quat(glm::radians(eulerAngles));
        glm::mat4 newTransform = glm::translate(glm::mat4(1.0f), position) *
                                 glm::mat4_cast(rotation) *
                                 glm::scale(glm::mat4(1.0f), scale);
        currentModel_.transform = newTransform;

        Operation op;
        op.execute = [this, uuid = currentModel_.uuid, newTransform]() {
            eventBus_->Publish(Events::ModelTransformedEvent{uuid, newTransform});
        };
        op.undo = [this, uuid = currentModel_.uuid, oldTransform]() {
            eventBus_->Publish(Events::ModelTransformedEvent{uuid, oldTransform});
        };
        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        eventBus_->Publish(Events::ModelTransformedEvent{currentModel_.uuid, newTransform});
    }

    if (ImGui::DragFloat3(u8"缩放", &scale[0], 0.01f)) {
        glm::mat4 oldTransform = currentModel_.transform;
        glm::mat4 newTransform = glm::translate(glm::mat4(1.0f), position) *
                                 glm::mat4_cast(rotation) *
                                 glm::scale(glm::mat4(1.0f), scale);
        currentModel_.transform = newTransform;

        Operation op;
        op.execute = [this, uuid = currentModel_.uuid, newTransform]() {
            eventBus_->Publish(Events::ModelTransformedEvent{uuid, newTransform});
        };
        op.undo = [this, uuid = currentModel_.uuid, oldTransform]() {
            eventBus_->Publish(Events::ModelTransformedEvent{uuid, oldTransform});
        };
        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        eventBus_->Publish(Events::ModelTransformedEvent{currentModel_.uuid, newTransform});
    }
}

void ControlPanel::RenderMaterialProperties() {
    ImGui::Separator();
    ImGui::Text(u8"材质属性");
    auto material = materialManager_->GetMaterial(currentMaterial_.uuid);
    if (!material) {
        ImGui::Text(u8"未分配材质");
        return;
    }

    glm::vec3 diffuse = currentMaterial_.diffuseColor;
    if (ImGui::ColorEdit3(u8"漫反射颜色", &diffuse[0])) {
        MaterialData oldData = currentMaterial_;
        currentMaterial_.diffuseColor = diffuse;
        Operation op;
        op.execute = [this, uuid = currentMaterial_.uuid, data = currentMaterial_]() {
            materialManager_->UpdateMaterial(uuid, data);
        };
        op.undo = [this, uuid = currentMaterial_.uuid, oldData]() {
            materialManager_->UpdateMaterial(uuid, oldData);
        };
        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        materialManager_->UpdateMaterial(currentMaterial_.uuid, currentMaterial_);
    }

    glm::vec3 specular = currentMaterial_.specularColor;
    if (ImGui::ColorEdit3(u8"镜面反射颜色", &specular[0])) {
        MaterialData oldData = currentMaterial_;
        currentMaterial_.specularColor = specular;
        Operation op;
        op.execute = [this, uuid = currentMaterial_.uuid, data = currentMaterial_]() {
            materialManager_->UpdateMaterial(uuid, data);
        };
        op.undo = [this, uuid = currentMaterial_.uuid, oldData]() {
            materialManager_->UpdateMaterial(uuid, oldData);
        };
        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        materialManager_->UpdateMaterial(currentMaterial_.uuid, currentMaterial_);
    }

    float shininess = currentMaterial_.shininess;
    if (ImGui::DragFloat(u8"光泽度", &shininess, 1.0f, 0.0f, 128.0f)) {
        MaterialData oldData = currentMaterial_;
        currentMaterial_.shininess = shininess;
        Operation op;
        op.execute = [this, uuid = currentMaterial_.uuid, data = currentMaterial_]() {
            materialManager_->UpdateMaterial(uuid, data);
        };
        op.undo = [this, uuid = currentMaterial_.uuid, oldData]() {
            materialManager_->UpdateMaterial(uuid, oldData);
        };
        eventBus_->Publish(Events::PushUndoOperationEvent{op});
        materialManager_->UpdateMaterial(currentMaterial_.uuid, currentMaterial_);
    }
}

void ControlPanel::RenderShaderInfo() {
    ImGui::Separator();
    ImGui::Text(u8"着色器信息");
    auto material = materialManager_->GetMaterial(currentMaterial_.uuid);
    if (material) {
        std::string vertexPath = material->GetVertexShaderPath();
        std::string fragmentPath = material->GetFragmentShaderPath();
        ImGui::Text(u8"顶点着色器: %s", vertexPath.c_str());
        if (ImGui::Button(u8"打开顶点着色器")) {
            eventBus_->Publish(Events::OpenFileEvent{vertexPath});
        }
        ImGui::Text(u8"片段着色器: %s", fragmentPath.c_str());
        if (ImGui::Button(u8"打开片段着色器")) {
            eventBus_->Publish(Events::OpenFileEvent{fragmentPath});
        }
        ImGui::Text(u8"编译状态: %s", shaderCompileStatus_.c_str());
    }
}

void ControlPanel::RenderTextureSelector() {
    ImGui::Separator();
    ImGui::Text(u8"纹理");
    auto texture = textureManager_->GetTexture(currentMaterial_.textureUUID);
    if (texture) {
        ImGui::Text(u8"当前纹理: %s", texture->GetFilepath().c_str());
        ImGui::Image((ImTextureID)(intptr_t)texture->GetTextureID(), ImVec2(100, 100));
    } else {
        ImGui::Text(u8"未分配纹理");
    }
    if (ImGui::Button(u8"选择纹理")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog("ChooseTextureDlg", "选择纹理", ".png,.jpg,.jpeg", config);
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseTextureDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string newFilepath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string oldTextureUUID = currentMaterial_.textureUUID;

            Operation op;
            op.execute = [this, newFilepath]() {
                std::string textureUUID = textureManager_->LoadTexture(newFilepath);
                currentMaterial_.textureUUID = textureUUID;
                materialManager_->UpdateMaterial(currentMaterial_.uuid, currentMaterial_);
            };
            op.undo = [this, oldTextureUUID]() {
                currentMaterial_.textureUUID = oldTextureUUID;
                materialManager_->UpdateMaterial(currentMaterial_.uuid, currentMaterial_);
            };
            eventBus_->Publish(Events::PushUndoOperationEvent{op});

            std::string textureUUID = textureManager_->LoadTexture(newFilepath);
            currentMaterial_.textureUUID = textureUUID;
            materialManager_->UpdateMaterial(currentMaterial_.uuid, currentMaterial_);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void ControlPanel::OnModelSelectionChanged(const Events::ModelSelectionChangedEvent& event) {
    hasSelectedModel_ = true;
    currentModel_.uuid = event.modelUUID;
    SetSelectedModel(currentModel_);
}

void ControlPanel::OnOperationModeChanged(const OperationModeChangedEvent& event) {
    switch (event.mode) {
        case OperationModeChangedEvent::Mode::Vertex:
            ImGui::Text(u8"顶点模式: %zu 个顶点", currentModel_.vertices.size());
            break;
        case OperationModeChangedEvent::Mode::Edge:
            ImGui::Text(u8"边模式: 已启用边工具");
            break;
        case OperationModeChangedEvent::Mode::Face:
            ImGui::Text(u8"面模式: 已启用面工具");
            break;
        case OperationModeChangedEvent::Mode::Object:
            ImGui::Text(u8"对象模式");
            break;
    }
}

void ControlPanel::OnProjectLoadFailed(const Events::ProjectLoadFailedEvent& event) {
    hasSelectedModel_ = false;
    currentModel_ = ModelData{};
    currentMaterial_ = MaterialData{};
    currentTexturePath_ = "";
}

void ControlPanel::OnModelDeleted(const Events::ModelDeletedEvent& event) {
    if (event.modelUUID == currentModel_.uuid) {
        hasSelectedModel_ = false;
        currentModel_ = ModelData{};
        currentMaterial_ = MaterialData{};
        currentTexturePath_ = "";
    }
}

void ControlPanel::OnMaterialUpdated(const Events::MaterialUpdatedEvent& event) {
    if (event.materialUUID == currentMaterial_.uuid) {
        currentMaterial_ = MaterialData{
            event.materialUUID,
            event.diffuseColor,
            event.specularColor,
            event.shininess,
            event.textureUUID
        };
    }
}

void ControlPanel::OnTextureLoaded(const Events::TextureLoadedEvent& event) {
    if (event.success && event.uuid == currentMaterial_.textureUUID) {
        currentTexturePath_ = event.filepath;
    }
}

} // namespace MyRenderer