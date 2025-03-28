#include "ProceduralWindow.h"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

ProceduralWindow::ProceduralWindow(std::shared_ptr<EventBus> eventBus, std::shared_ptr<ModelLoader> modelLoader)
    : eventBus_(eventBus), modelLoader_(modelLoader), generationProgress_(0.0f), isGenerating_(false) {
    if (!eventBus_) throw std::invalid_argument("ProceduralWindow: EventBus cannot be null");
    if (!modelLoader_) throw std::invalid_argument("ProceduralWindow: ModelLoader cannot be null");
    LoadAlgorithmsFromConfig();
    SubscribeToEvents();
}

ProceduralWindow::~ProceduralWindow() {}

void ProceduralWindow::LoadAlgorithmsFromConfig() {
    const fs::path configPath = "Core/Config/ProceduralRegister.json";
    try {
        nlohmann::json config = JSONSerializer::DeserializeFromFile<nlohmann::json>(configPath);
        if (!config.contains("algorithms") || !config["algorithms"].is_array()) {
            throw std::runtime_error("Invalid ProceduralRegister.json format");
        }

        for (const auto& algo : config["algorithms"]) {
            std::string name = algo.value("name", "");
            if (name.empty() || !algo.contains("parameters")) continue;

            algorithmTemplates_[name] = algo["parameters"];
            availableAlgorithms_.push_back(name);

            if (currentAlgorithm_.empty()) {
                currentAlgorithm_ = name;
                currentParams_ = algo["parameters"];
                for (auto& [key, value] : currentParams_.items()) {
                    value = value["default"];
                }
            }
        }

        if (availableAlgorithms_.empty()) {
            throw std::runtime_error("No algorithms found in ProceduralRegister.json");
        }
    } catch (const std::exception& e) {
        resultMessage_ = std::string("Failed to load config: ") + e.what();
    }
}

void ProceduralWindow::SaveToConfig() {
    const fs::path configPath = "Core/config/ProceduralRegister.json";
    try {
        nlohmann::json config;
        config["algorithms"] = nlohmann::json::array();
        for (const auto& algoName : availableAlgorithms_) {
            nlohmann::json algo;
            algo["name"] = algoName;
            algo["parameters"] = (algoName == currentAlgorithm_) ? currentParams_ : algorithmTemplates_[algoName];
            config["algorithms"].push_back(algo);
        }
        JSONSerializer::SerializeToFile(config, configPath);
    } catch (const std::exception& e) {
        resultMessage_ = std::string("Failed to save config: ") + e.what();
    }
}

void ProceduralWindow::SubscribeToEvents() {
    eventBus_->Subscribe<MyRenderer::Events::ProceduralGenerationStartedEvent>(
        [this](const MyRenderer::Events::ProceduralGenerationStartedEvent&) {
            isGenerating_ = true;
            generationProgress_ = 0.0f;
            resultMessage_ = "Generation started...";
        });

    eventBus_->Subscribe<MyRenderer::Events::ProgressUpdateEvent>(
        [this](const MyRenderer::Events::ProgressUpdateEvent& event) {
            generationProgress_ = event.progress;
        });

    eventBus_->Subscribe<MyRenderer::Events::ProceduralGenerationCompletedEvent>(
        [this](const MyRenderer::Events::ProceduralGenerationCompletedEvent& event) {
            isGenerating_ = false;
            if (event.success) {
                resultMessage_ = "Generation completed successfully! UUID: " + event.modelData.uuid;
            } else {
                resultMessage_ = "Generation failed: " + event.errorMessage;
            }
        });

    eventBus_->Subscribe<MyRenderer::Events::ProceduralGenerationStoppedEvent>(
        [this](const MyRenderer::Events::ProceduralGenerationStoppedEvent&) {
            isGenerating_ = false;
            resultMessage_ = "Generation stopped.";
        });
}

void ProceduralWindow::Update() {
    ImGui::Begin("Procedural Generator", nullptr, ImGuiWindowFlags_NoCollapse);

    RenderAlgorithmSelector();
    RenderParameters();
    RenderProgress();

    ImGui::End();
}

void ProceduralWindow::Generate() {
    SaveToConfig(); // 保存参数到 JSON
    eventBus_->Publish(MyRenderer::Events::RequestModelCreatedEvent{currentAlgorithm_, currentParams_});
}

void ProceduralWindow::UpdateProgress(float progress) {
    generationProgress_ = progress;
}

void ProceduralWindow::RenderAlgorithmSelector() {
    if (ImGui::CollapsingHeader("Algorithm Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int selectedIndex = 0;
        std::vector<const char*> items;
        for (const auto& algo : availableAlgorithms_) {
            items.push_back(algo.c_str());
        }
        if (ImGui::Combo("Algorithm", &selectedIndex, items.data(), items.size())) {
            currentAlgorithm_ = availableAlgorithms_[selectedIndex];
            currentParams_ = algorithmTemplates_[currentAlgorithm_];
            for (auto& [key, value] : currentParams_.items()) {
                value = value["default"];
            }
        }
    }
}

void ProceduralWindow::RenderParameters() {
    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (algorithmTemplates_.find(currentAlgorithm_) != algorithmTemplates_.end()) {
            GenerateParameterUI(currentParams_, algorithmTemplates_[currentAlgorithm_]);
        } else {
            ImGui::Text("No algorithm selected.");
        }

        ImGui::BeginDisabled(isGenerating_);
        if (ImGui::Button("Generate")) {
            Generate();
        }
        ImGui::EndDisabled();

        if (isGenerating_ && ImGui::Button("Cancel")) {
            eventBus_->Publish(MyRenderer::Events::RequestGenerationCancelEvent{});
        }
    }
}

void ProceduralWindow::RenderProgress() {
    if (ImGui::CollapsingHeader("Progress", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (isGenerating_) {
            ImGui::ProgressBar(generationProgress_, ImVec2(0.0f, 0.0f));
        }
        ImGui::Text("%s", resultMessage_.c_str());
    }
}

void ProceduralWindow::GenerateParameterUI(nlohmann::json& params, const nlohmann::json& templateParams) {
    for (auto& [key, value] : templateParams.items()) {
        std::string type = value.value("type", "string");
        if (type == "int") {
            int val = params[key].get<int>();
            if (ImGui::InputInt(key.c_str(), &val)) {
                params[key] = val;
            }
        } else if (type == "float") {
            float val = params[key].get<float>();
            if (ImGui::InputFloat(key.c_str(), &val)) {
                params[key] = val;
            }
        } else if (type == "string") {
            std::string val = params[key].get<std::string>();
            char buffer[256];
            strncpy_s(buffer, val.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';
            if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
                params[key] = std::string(buffer);
            }
        } else if (type == "array" && key == "tileSet") {
            if (!params[key].is_array()) params[key] = value["default"];
            ImGui::Text("Tile Set");
            std::vector<std::string> array = params[key].get<std::vector<std::string>>();
            for (int i = 0; i < static_cast<int>(array.size()); ++i) {
                char buffer[256];
                strncpy_s(buffer, array[i].c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::InputText("Path", buffer, sizeof(buffer))) {
                    array[i] = std::string(buffer);
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove")) {
                    array.erase(array.begin() + i);
                    --i;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Tile")) {
                array.push_back("new_tile.obj");
            }
            params[key] = array;
        } else if (type == "object" && key == "adjacencyRules") {
            if (!params[key].is_object()) params[key] = value["default"];
            ImGui::Text("Adjacency Rules");
            std::vector<std::string> tileSet = params["tileSet"].get<std::vector<std::string>>();
            for (const auto& tile : tileSet) {
                if (!params[key].contains(tile)) params[key][tile] = nlohmann::json::array();
                std::vector<std::string> adjacent = params[key][tile].get<std::vector<std::string>>();
                if (ImGui::TreeNode(tile.c_str())) {
                    for (const auto& otherTile : tileSet) {
                        bool isAdjacent = std::find(adjacent.begin(), adjacent.end(), otherTile) != adjacent.end();
                        bool newIsAdjacent = isAdjacent;
                        ImGui::Checkbox(otherTile.c_str(), &newIsAdjacent);
                        if (newIsAdjacent != isAdjacent) {
                            if (newIsAdjacent) {
                                adjacent.push_back(otherTile);
                            } else {
                                adjacent.erase(std::remove(adjacent.begin(), adjacent.end(), otherTile), adjacent.end());
                            }
                            params[key][tile] = adjacent;
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
}