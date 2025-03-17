#include "MenuBar.h"
#include <imgui.h>

void MenuBar::Draw() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                // 处理新建项目
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}