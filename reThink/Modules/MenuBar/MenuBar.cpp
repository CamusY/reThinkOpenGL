#include "MenuBar.h"
#include <iostream>
#include <GLFW/glfw3.h>

#include "imgui-docking/ImGuiFileDialog.h"
#include "ModelLoader/ModelLoader.h"

namespace MyRenderer {

MenuBar::MenuBar(std::shared_ptr<EventBus> eventBus, ConfigManager& configManager)
    : eventBus_(eventBus), configManager_(configManager), showErrorPopup_(false) {
    // 订阅ProjectLoadFailedEvent
    eventBus_->Subscribe<Events::ProjectLoadFailedEvent>([this](const Events::ProjectLoadFailedEvent& event) {
        errorMessage_ = event.errorMsg;
        showErrorPopup_ = true;
    });
}

MenuBar::~MenuBar() {}

void MenuBar::Update() {
    DrawMenuBar();

    // 显示错误弹窗
    if (showErrorPopup_) {
        ShowErrorPopup();
    }
}

void MenuBar::DrawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        HandleFileMenu();
        HandleEditMenu();
        HandleViewMenu();
        HandleImportMenu();
        ImGui::EndMainMenuBar();
    }
}

void MenuBar::HandleFileMenu() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("NewProjectDlg", "Choose Project Directory", nullptr, config);
        }

        if (ImGui::MenuItem("Open Project")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.fileName = "project.proj"; // 设置默认文件名
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("OpenProjectDlg", "Choose Project File", ".proj", config);
        }

        if (ImGui::MenuItem("Save Project")) {
            eventBus_->Publish(RequestSaveProjectEvent{false, ""});
        }

        if (ImGui::MenuItem("Save Project As...")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.fileName = "project.proj"; // 设置默认文件名
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("SaveProjectDlg", "Save Project As", ".proj", config);
        }

        if (ImGui::MenuItem("Exit")) {
            glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
        }

        ImGui::EndMenu();
    }

    // 处理新建项目对话框
    if (ImGuiFileDialog::Instance()->Display("NewProjectDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string projectDir = ImGuiFileDialog::Instance()->GetCurrentPath();
            std::string projectName = ImGuiFileDialog::Instance()->GetFilePathName();
            if (projectName.empty()) projectName = "NewProject";
            eventBus_->Publish(RequestNewProjectEvent{projectName, projectDir});
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // 处理打开项目对话框
    if (ImGuiFileDialog::Instance()->Display("OpenProjectDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            eventBus_->Publish(RequestOpenProjectEvent{filePath});
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // 处理另存为对话框
    if (ImGuiFileDialog::Instance()->Display("SaveProjectDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            eventBus_->Publish(RequestSaveProjectEvent{true, filePath});
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void MenuBar::HandleEditMenu() {
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
            eventBus_->Publish(Events::UndoRedoEvent{Events::UndoRedoEvent::Action::Undo});
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
            eventBus_->Publish(Events::UndoRedoEvent{Events::UndoRedoEvent::Action::Redo});
        }
        ImGui::EndMenu();
    }
}

void MenuBar::HandleViewMenu() {
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Modeling Layout")) {
            eventBus_->Publish(LayoutChangeEvent{"modeling"});
        }
        if (ImGui::MenuItem("Rendering Layout")) {
            eventBus_->Publish(LayoutChangeEvent{"rendering"});
        }
        if (ImGui::MenuItem("Save Current Layout")) {
            IGFD::FileDialogConfig config;
            config.path = "Config/layouts/";
            config.fileName = "layout.ini"; // 设置默认布局文件名
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("SaveLayoutDlg", "Save Layout As", ".ini", config);
        }
        ImGui::EndMenu();
    }

    // 处理保存布局对话框
    if (ImGuiFileDialog::Instance()->Display("SaveLayoutDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string layoutPath = ImGuiFileDialog::Instance()->GetFilePathName();
            size_t pos = layoutPath.find("Config/layouts/");
            if (pos != std::string::npos) {
                std::string layoutName = layoutPath.substr(pos + 15, layoutPath.size() - pos - 19); // 去掉路径和.ini
                configManager_.SaveCurrentLayout(layoutName);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void MenuBar::HandleImportMenu() {
    if (ImGui::BeginMenu("Import")) {
        if (ImGui::MenuItem("Import Model")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.fileName = "model.gltf"; // 设置默认模型文件名
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("ImportModelDlg", "Choose Model File", ".gltf,.obj", config);
        }
        ImGui::EndMenu();
    }

    // 处理导入模型对话框
    if (ImGuiFileDialog::Instance()->Display("ImportModelDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            eventBus_->Publish(Events::ModelLoadedEvent{ModelData{filePath, "ImportedModel"}}); // 简化，实际应由ModelLoader处理
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void MenuBar::ShowErrorPopup() {
    ImGui::OpenPopup("Error");
    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Error: %s", errorMessage_.c_str());
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            showErrorPopup_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace MyRenderer