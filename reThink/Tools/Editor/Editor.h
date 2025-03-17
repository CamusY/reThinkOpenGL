// Editor.h
#pragma once
#include "../Model/Model.h"
#include <./imgui-docking/imgui.h>

class Editor {
    enum EditMode { NONE, VERTEX, EDGE, FACE };
    
    Model* model;
    EditMode currentMode = NONE;
    bool editMode = false;
    int selectedVertex = -1;
    
public:
    Editor(Model* model) : model(model) {}
    
    void toggleEditMode() { editMode = !editMode; }
    void handleViewportInteraction(const ImVec2& viewportPos, const ImVec2& viewportSize);
    void drawPropertiesPanel();
};