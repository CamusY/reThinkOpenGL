#pragma once
#include "IProceduralGenerator/IProceduralGenerator.h"
#include "ModelLoader/ModelLoader.h"
#include <vector>
#include <atomic>
#include <thread>
#include <map>

class WFCGenerator : public IProceduralGenerator {
public:
    WFCGenerator(std::shared_ptr<ModelLoader> modelLoader);
    ~WFCGenerator() override;

    void Generate(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus) override;
    void Cancel() override;
    std::string GetName() const override { return "WFC"; }

private:
    struct Cell {
        std::vector<int> possibleTiles; // 可用的瓦片索引
        bool collapsed = false;
    };

    void RunGeneration(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus);
    ModelData GenerateGrid(const std::vector<std::string>& tileSet, const nlohmann::json& adjacencyRules, int width, int height, int depth);
    void Collapse(std::vector<std::vector<std::vector<Cell>>>& grid, int x, int y, int z, const std::vector<std::string>& tileSet, const nlohmann::json& adjacencyRules);
    void Propagate(std::vector<std::vector<std::vector<Cell>>>& grid, int x, int y, int z, const std::vector<std::string>& tileSet, const nlohmann::json& adjacencyRules);

    std::shared_ptr<ModelLoader> modelLoader_;
    std::atomic<bool> cancelled_;
    std::thread generationThread_;
};