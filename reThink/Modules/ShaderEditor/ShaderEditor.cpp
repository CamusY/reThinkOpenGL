#include "ShaderEditor.h"
#include "imgui.h"
#include <fstream>
#include <sstream>

ShaderEditor::ShaderEditor(std::shared_ptr<EventBus> eventBus)
    : eventBus_(std::move(eventBus)), isDirty_(false) {
    // 设置编辑器为 GLSL 模式
    editor_.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
    editor_.SetPalette(TextEditor::GetDarkPalette());

    // 订阅 OpenFileEvent
    eventBus_->Subscribe<MyRenderer::Events::OpenFileEvent>(
        [this](const MyRenderer::Events::OpenFileEvent& event) {
            OpenFile(event.filepath);
        });

    // 订阅 ShaderCompiledEvent 以显示编译结果
    eventBus_->Subscribe<MyRenderer::Events::ShaderCompiledEvent>(
        [this](const MyRenderer::Events::ShaderCompiledEvent& event) {
            if (!event.success) {
                editor_.SetErrorMarkers({{1, event.errorMessage}});
            } else {
                editor_.SetErrorMarkers({}); // 清除错误标记
            }
        });

    // 加载默认着色器
    LoadDefaultShaders();
}

void ShaderEditor::Update() {
    ImGui::Begin("Shader Editor", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Vertex Shader")) {
                NewFile(".vs");
            }
            if (ImGui::MenuItem("New Fragment Shader")) {
                NewFile(".fs");
            }
            if (ImGui::MenuItem("Open")) {
                showFileDialog_ = true;
                IGFD::FileDialogConfig config;
                config.path = "."; // 设置初始路径
                ImGuiFileDialog::Instance()->OpenDialog("OpenShaderDlg", "Choose Shader File", 
                                                      ".vs,.fs", config);
            }
            if (ImGui::MenuItem("Save", nullptr, nullptr, isDirty_)) {
                SaveFile();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Build")) {
            if (ImGui::MenuItem("Compile")) {
                CompileShader();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // 文件对话框处理
    if (showFileDialog_ && ImGuiFileDialog::Instance()->Display("OpenShaderDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
            OpenFile(filepath);
        }
        ImGuiFileDialog::Instance()->Close();
        showFileDialog_ = false;
    }

    editor_.Render("ShaderEditor", ImGui::GetContentRegionAvail(), true);
    isDirty_ = editor_.IsTextChanged();

    ImGui::End();
}

void ShaderEditor::OpenFile(const std::string& filepath) {
    std::string content = ReadFile(filepath);
    if (!content.empty()) {
        editor_.SetText(content);
        currentFilepath_ = filepath;
        isDirty_ = false;
    }
}

void ShaderEditor::LoadDefaultShaders() {
    // 默认顶点着色器
    const std::string defaultVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
})";
    // 默认片段着色器
    const std::string defaultFragmentShader = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})";

    if (currentFilepath_.empty() || currentFilepath_.find(".vs") != std::string::npos) {
        editor_.SetText(defaultVertexShader);
        currentFilepath_ = "default.vs";
    } else if (currentFilepath_.find(".fs") != std::string::npos) {
        editor_.SetText(defaultFragmentShader);
        currentFilepath_ = "default.fs";
    }
}

void ShaderEditor::SaveFile() {
    if (currentFilepath_.empty()) {
        // 使用 std::string 进行拼接
        currentFilepath_ = std::string("untitled") + (editor_.GetText().find("in vec3") != std::string::npos ? ".vs" : ".fs");
    }
    std::ofstream file(currentFilepath_);
    if (file.is_open()) {
        file << editor_.GetText();
        file.close();
        isDirty_ = false;
    }
}

void ShaderEditor::CompileShader() {
    SaveFile(); // 保存当前编辑内容
    std::string vertexPath = currentFilepath_.find(".vs") != std::string::npos ? currentFilepath_ : "";
    std::string fragmentPath = currentFilepath_.find(".fs") != std::string::npos ? currentFilepath_ : "";
    
    // 如果当前是顶点着色器，假设对应的片段着色器路径为替换扩展名
    if (!vertexPath.empty() && fragmentPath.empty()) {
        fragmentPath = vertexPath.substr(0, vertexPath.find(".vs")) + ".fs";
    } else if (!fragmentPath.empty() && vertexPath.empty()) {
        vertexPath = fragmentPath.substr(0, fragmentPath.find(".fs")) + ".vs";
    }

    // 发布编译请求
    eventBus_->Publish(MyRenderer::Events::ShaderCompileRequestEvent{vertexPath, fragmentPath});
}

void ShaderEditor::NewFile(const std::string& extension) {
    currentFilepath_ = "new_shader" + extension;
    if (extension == ".vs") {
        editor_.SetText(R"(
#version 330 core
layout(location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
})");
    } else if (extension == ".fs") {
        editor_.SetText(R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})");
    }
    isDirty_ = true;
}

std::string ShaderEditor::ReadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}