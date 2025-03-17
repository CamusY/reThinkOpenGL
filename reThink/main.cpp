#include "Window.h"
#include "SceneViewport.h"
#include "ControlPanel.h"
#include "MenuBar.h"
#include "Model.h"

int main() {
    Window mainWindow;
    mainWindow.Init(1280, 720, "MyRenderer");
    
    Model model;
    SceneViewport viewport;
    ControlPanel panel;
    MenuBar menuBar;
    
    panel.SetTarget(&model);
    
    while (!glfwWindowShouldClose(mainWindow.GetNativeWindow())) {
        mainWindow.PreRender();
        
        menuBar.Draw();
        panel.Draw();
        viewport.Render(&model);
        
        mainWindow.PostRender();
        glfwPollEvents();
    }
    return 0;
}