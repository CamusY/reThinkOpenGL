#ifndef MENUBAR_H
#define MENUBAR_H

#include <memory>
#include <string>
#include "EventBus/EventBus.h"
#include "Config/ConfigManager.h"
#include "imgui-docking/imgui.h"

namespace MyRenderer {

    struct RequestNewProjectEvent {
        std::string projectName;
        std::string projectDir;
    };

    struct RequestOpenProjectEvent {
        std::string filePath;
    };

    struct RequestSaveProjectEvent {
        bool saveAs;
        std::string filePath;
    };

    struct LayoutChangeEvent {
        std::string layoutName;
    };

    class MenuBar {
    public:
        MenuBar(std::shared_ptr<EventBus> eventBus, ConfigManager& configManager);
        ~MenuBar();

        void Update();

    private:
        void DrawMenuBar();
        void HandleFileMenu();
        void HandleEditMenu();
        void HandleViewMenu();
        void HandleImportMenu();
        void ShowErrorPopup();

        std::shared_ptr<EventBus> eventBus_;
        ConfigManager& configManager_;
        bool showErrorPopup_;
        std::string errorMessage_;
    };

} // namespace MyRenderer

#endif // MENUBAR_H