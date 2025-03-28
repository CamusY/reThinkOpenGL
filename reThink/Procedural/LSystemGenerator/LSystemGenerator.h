#pragma once
#include "IProceduralGenerator/IProceduralGenerator.h"
#include <vector>
#include <atomic>
#include <thread>

class LSystemGenerator : public IProceduralGenerator {
public:
    LSystemGenerator();
    ~LSystemGenerator() override;

    void Generate(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus) override;
    void Cancel() override;
    std::string GetName() const override { return "LSystem"; }

private:
    void RunGeneration(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus);
    ModelData GenerateTree(int iterations, float length, float angle);
    void BuildBranch(std::vector<glm::vec3>& vertices, glm::vec3 pos, float length, float angle, int depth);

    std::atomic<bool> cancelled_;
    std::thread generationThread_;
};