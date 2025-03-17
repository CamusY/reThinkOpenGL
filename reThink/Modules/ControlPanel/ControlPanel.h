#pragma once
#include "./Model/Model.h"
#include "../Core/EventBus/EventBus.h"

class ControlPanel {
    Model* currentModel;
    EventBus& eventBus;
    
public:
    ControlPanel(Model* model, EventBus& bus);
    void Draw();
};