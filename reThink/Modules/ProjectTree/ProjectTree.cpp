#include "ProjectTree.h"
#include <imgui.h>

namespace MyRenderer {

ProjectTree::ProjectTree(std::shared_ptr<EventBus> eventBus)
    : eventBus_(eventBus) {
    SubscribeToEvents();
}

ProjectTree::~ProjectTree() {}

void ProjectTree::Update() {
    if (!projectData_.projectName.empty()) {
        ImGui::Begin("Project Tree", nullptr, ImGuiWindowFlags_NoCollapse);
        
        // 渲染根节点（项目名称）
        if (ImGui::TreeNodeEx(projectData_.projectName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            // 渲染所有无父节点的模型（根模型）
            for (const auto& model : projectData_.models) {
                if (model.parentUUID.empty()) {
                    RenderTreeNode(model.uuid, hierarchy_);
                }
            }
            ImGui::TreePop();
        }
        
        ImGui::End();
    }
}

void ProjectTree::SetProjectData(const ProjectData& data) {
    projectData_ = data;
    modelMap_.clear();
    hierarchy_.clear();

    // 构建模型映射和层级结构
    for (const auto& model : projectData_.models) {
        modelMap_[model.uuid] = model;
        if (!model.parentUUID.empty()) {
            hierarchy_[model.parentUUID].push_back(model.uuid);
        }
    }
}

void ProjectTree::SubscribeToEvents() {
    eventBus_->Subscribe<Events::ProjectOpenedEvent>(
        [this](const Events::ProjectOpenedEvent& event) { OnProjectOpened(event); },
        EventBus::Priority::Normal
    );
    eventBus_->Subscribe<Events::ModelDeletedEvent>(
        [this](const Events::ModelDeletedEvent& event) { OnModelDeleted(event); },
        EventBus::Priority::High
    );
}

void ProjectTree::OnProjectOpened(const Events::ProjectOpenedEvent& event) {
    // 假设 ProjectManager 会通过其他方式提供 ProjectData，这里仅清空旧数据
    modelMap_.clear();
    hierarchy_.clear();
    selectedModelUUID_.clear();
}

void ProjectTree::OnModelDeleted(const Events::ModelDeletedEvent& event) {
    auto it = modelMap_.find(event.modelUUID);
    if (it != modelMap_.end()) {
        // 从层级结构中移除
        if (!it->second.parentUUID.empty()) {
            auto& siblings = hierarchy_[it->second.parentUUID];
            siblings.erase(
                std::remove(siblings.begin(), siblings.end(), event.modelUUID),
                siblings.end()
            );
            if (siblings.empty()) {
                hierarchy_.erase(it->second.parentUUID);
            }
        }
        // 从子节点映射中移除
        hierarchy_.erase(event.modelUUID);
        // 从模型映射中移除
        modelMap_.erase(event.modelUUID);
    }
}

void ProjectTree::RenderTreeNode(const std::string& modelUUID, const std::map<std::string, std::vector<std::string>>& hierarchy) {
    auto it = modelMap_.find(modelUUID);
    if (it == modelMap_.end()) return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedModelUUID_ == modelUUID) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool hasChildren = hierarchy.find(modelUUID) != hierarchy.end();
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool nodeOpen = ImGui::TreeNodeEx(modelUUID.c_str(), flags, "%s", it->second.filepath.c_str());

    // 处理选择
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedModelUUID_ = modelUUID;
        eventBus_->Publish(Events::ModelSelectionChangedEvent{modelUUID});
    }

    // 处理拖拽
    HandleDragDrop(modelUUID);

    // 处理右键菜单
    HandleContextMenu(modelUUID);

    // 渲染子节点
    if (nodeOpen && hasChildren) {
        for (const auto& childUUID : hierarchy.at(modelUUID)) {
            RenderTreeNode(childUUID, hierarchy);
        }
        ImGui::TreePop();
    }
}

void ProjectTree::HandleDragDrop(const std::string& modelUUID) {
    // 拖拽源
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("MODEL_UUID", modelUUID.c_str(), modelUUID.size() + 1);
        ImGui::Text("Move %s", modelMap_[modelUUID].filepath.c_str());
        ImGui::EndDragDropSource();
    }

    // 拖拽目标
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_UUID")) {
            std::string draggedUUID(static_cast<const char*>(payload->Data), payload->DataSize - 1);
            if (draggedUUID != modelUUID && modelMap_.find(draggedUUID) != modelMap_.end()) {
                // 更新父子关系
                ModelData& draggedModel = modelMap_[draggedUUID];
                if (!draggedModel.parentUUID.empty()) {
                    auto& oldSiblings = hierarchy_[draggedModel.parentUUID];
                    oldSiblings.erase(
                        std::remove(oldSiblings.begin(), oldSiblings.end(), draggedUUID),
                        oldSiblings.end()
                    );
                    if (oldSiblings.empty()) {
                        hierarchy_.erase(draggedModel.parentUUID);
                    }
                }
                draggedModel.parentUUID = modelUUID;
                hierarchy_[modelUUID].push_back(draggedUUID);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void ProjectTree::HandleContextMenu(const std::string& modelUUID) {
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Locate to Control Panel")) {
            eventBus_->Publish(Events::ModelSelectionChangedEvent{modelUUID});
        }
        if (ImGui::MenuItem("Open Shader")) {
            const auto& model = modelMap_[modelUUID];
            if (!model.vertexShaderPath.empty()) {
                eventBus_->Publish(Events::OpenFileEvent{model.vertexShaderPath});
            } else if (!model.fragmentShaderPath.empty()) {
                eventBus_->Publish(Events::OpenFileEvent{model.fragmentShaderPath});
            }
        }
        ImGui::EndPopup();
    }
}

} // namespace MyRenderer