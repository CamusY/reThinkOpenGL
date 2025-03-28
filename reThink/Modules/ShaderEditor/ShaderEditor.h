#ifndef SHADER_EDITOR_H
#define SHADER_EDITOR_H

#include <string>
#include <memory>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "imgui-backends/TextEditor.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

class ShaderEditor {
public:
    ShaderEditor(std::shared_ptr<EventBus> eventBus);
    ~ShaderEditor() = default;

    // 更新 UI 和状态
    void Update();

    // 打开指定着色器文件
    void OpenFile(const std::string& filepath);

private:
    // 初始化默认着色器代码
    void LoadDefaultShaders();

    // 保存当前编辑的代码到文件
    void SaveFile();

    // 提交编译请求
    void CompileShader();

    // 处理新建文件
    void NewFile(const std::string& extension);

    // 读取文件内容到编辑器
    std::string ReadFile(const std::string& filepath);

    std::shared_ptr<EventBus> eventBus_; // 事件总线
    TextEditor editor_;                  // ImGuiColorTextEdit 的编辑器实例
    std::string currentFilepath_;        // 当前编辑的文件路径
    bool isDirty_;                       // 文件是否已修改未保存
    bool showFileDialog_;                // 是否显示文件对话框
};

#endif // SHADER_EDITOR_H