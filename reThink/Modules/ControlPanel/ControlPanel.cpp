#include "ControlPanel.h"
#include "EventBus.h"
#include <imgui.h>

void ControlPanel::Draw() {
    ImGui::Begin("Control Panel");
    
    if (targetModel) {
        ImGui::DragFloat3("Position", &targetModel->position[0], 0.1f);
        ImGui::DragFloat3("Rotation", &targetModel->rotation[0], 1.0f);
        ImGui::DragFloat3("Scale", &targetModel->scale[0], 0.01f);
        
        // 发布变换事件
        ModelTransformedEvent event{targetModel->getModelMatrix()};
        EventBus::Get().publish(event);
    }
    
    ImGui::End();
}