#ifndef WINDOW_H
#define WINDOW_H
#define IMGUI_DEFINE_MATH_OPERATORS
#include <memory>
#include <string>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "EventBus/EventBus.h"
#include "Config/ConfigManager.h"
#include <imgui.h>
#include "imgui-backends/imgui_impl_glfw.h"
#include "imgui-backends/imgui_impl_opengl3.h"
#include "ThreadPool/ThreadPool.h"
#include "Utils/JSONSerializer.h"

#include "MenuBar/MenuBar.h"
#include "ControlPanel/ControlPanel.h"
#include "SceneViewport/SceneViewport.h"
// #include "ShaderEditor/ShaderEditor.h"
#include "ProjectTree/ProjectTree.h"
#include "InputHandler/InputHandler.h"
#include "ProceduralWindow/ProceduralWindow.h"
#include "Animation/Animation.h"
#include "ProjectManager/ProjectManager.h"
#include "ShaderManager/ShaderManager.h"
#include "ModelLoader/ModelLoader.h"
#include "MaterialManager/MaterialManager.h"
#include "TextureManager/TextureManager.h"
#include "UndoRedoManager/UndoRedoManager.h"
#include "AnimationManager/AnimationManager.h"
// 前向声明所有模块
namespace MyRenderer {

class Window {
public:
    Window(std::shared_ptr<EventBus> eventBus);
    ~Window();

    void Initialize();
    void Update();
    void Shutdown();

    bool ShouldClose() const;
    GLFWwindow* GetGLFWWindow() const { return window_; } // 提供窗口指针给其他模块

private:
    void SetupImGui();
    void ApplyInitialLayout();
    void SubscribeToEvents();
    void InitializeModules();

    std::shared_ptr<EventBus> eventBus_;
    ConfigManager configManager_;
    LayoutConfig layoutConfig_;
    GLFWwindow* window_;
    ImGuiID dockSpaceId_;
    bool firstRun_;

    // 所有模块的实例
    std::shared_ptr<ThreadPool> threadPool_;
    std::shared_ptr<JSONSerializer> jsonSerializer_;
    std::shared_ptr<TextureManager> textureManager_;
    std::shared_ptr<MaterialManager> materialManager_;
    std::shared_ptr<ShaderManager> shaderManager_;
    std::shared_ptr<ModelLoader> modelLoader_;
    std::shared_ptr<MenuBar> menuBar_;
    std::shared_ptr<ControlPanel> controlPanel_;
    std::shared_ptr<SceneViewport> sceneViewport_;
//    std::shared_ptr<ShaderEditor> shaderEditor_;
    std::shared_ptr<ProjectTree> projectTree_;
    std::shared_ptr<InputHandler> inputHandler_;
    std::shared_ptr<ProceduralWindow> proceduralWindow_;
    std::shared_ptr<Animation> animation_;
    std::shared_ptr<ProjectManager> projectManager_;
    std::shared_ptr<UndoRedoManager> undoRedoManager_;
    std::shared_ptr<AnimationManager> animationManager_;
};

} // namespace MyRenderer

#endif // WINDOW_H