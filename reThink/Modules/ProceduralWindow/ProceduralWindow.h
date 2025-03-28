#pragma once
#include <imgui.h>
#include <map>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "EventBus/EventBus.h"
#include "Utils/JSONSerializer.h"
#include "ModelLoader/ModelLoader.h"

class ProceduralWindow {
public:
    ProceduralWindow(std::shared_ptr<EventBus> eventBus, std::shared_ptr<ModelLoader> modelLoader);
    ~ProceduralWindow();

    void Update();
    void Generate();
    void UpdateProgress(float progress);

private:
    void LoadAlgorithmsFromConfig();
    void SaveToConfig();
    void SubscribeToEvents();
    void RenderAlgorithmSelector();
    void RenderParameters();
    void RenderProgress();
    void GenerateParameterUI(nlohmann::json& params, const nlohmann::json& templateParams);

    std::shared_ptr<EventBus> eventBus_;
    std::shared_ptr<ModelLoader> modelLoader_;
    std::map<std::string, nlohmann::json> algorithmTemplates_;
    std::vector<std::string> availableAlgorithms_;
    std::string currentAlgorithm_;
    nlohmann::json currentParams_;
    float generationProgress_;
    bool isGenerating_;
    std::string resultMessage_;
};