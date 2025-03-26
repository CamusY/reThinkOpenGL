#ifndef WINDOW_H
#define WINDOW_H

#include <memory>
#include <string>
#include "EventBus/EventBus.h"
#include "Config/ConfigManager.h"
#include "GLFW/glfw3.h"
#include "imgui-docking/imgui.h"
#include "imgui-docking/imgui_impl_glfw.h"
#include "imgui-docking/imgui_impl_opengl3.h"

namespace MyRenderer {

    class Window {
    public:
        Window(std::shared_ptr<EventBus> eventBus);
        ~Window();

        void Initialize();
        void Update();
        void Shutdown();

        bool ShouldClose() const;

    private:
        void SetupImGui();
        void ApplyInitialLayout();
        void SubscribeToEvents();

        std::shared_ptr<EventBus> eventBus_;
        ConfigManager configManager_;
        LayoutConfig layoutConfig_;
        GLFWwindow* window_;
        ImGuiID dockSpaceId_;
        bool firstRun_;
    };

} // namespace MyRenderer

#endif // WINDOW_H