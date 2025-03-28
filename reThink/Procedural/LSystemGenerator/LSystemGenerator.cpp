#include "LSystemGenerator.h"
#include <chrono>
#include "Utils/MathUtils.h"

LSystemGenerator::LSystemGenerator() : cancelled_(false) {}

LSystemGenerator::~LSystemGenerator() {
    if (generationThread_.joinable()) {
        Cancel();
        generationThread_.join();
    }
}

void LSystemGenerator::Generate(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus) {
    cancelled_ = false;
    if (generationThread_.joinable()) {
        generationThread_.join();
    }
    generationThread_ = std::thread(&LSystemGenerator::RunGeneration, this, params, eventBus);
}

void LSystemGenerator::Cancel() {
    cancelled_ = true;
}

void LSystemGenerator::RunGeneration(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus) {
    eventBus->Publish(MyRenderer::Events::ProceduralGenerationStartedEvent{});

    int iterations = params.value("iterations", 3);
    float length = params.value("length", 1.0f);
    float angle = params.value("angle", 30.0f);

    ModelData modelData = GenerateTree(iterations, length, angle);

    for (int i = 0; i <= 100; i += 10) {
        if (cancelled_) {
            eventBus->Publish(MyRenderer::Events::ProceduralGenerationStoppedEvent{});
            return;
        }
        eventBus->Publish(MyRenderer::Events::ProgressUpdateEvent{static_cast<float>(i) / 100.0f});
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟耗时
    }

    if (!cancelled_) {
        eventBus->Publish(MyRenderer::Events::ProceduralGenerationCompletedEvent{true, "", modelData});
    }
}

ModelData LSystemGenerator::GenerateTree(int iterations, float length, float angle) {
    ModelData modelData;
    modelData.uuid = "lsystem_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    modelData.filepath = "";
    modelData.transform = glm::mat4(1.0f);
    modelData.vertexShaderPath = "";
    modelData.fragmentShaderPath = "";
    modelData.parentUUID = "";

    std::vector<glm::vec3> vertices;
    BuildBranch(vertices, glm::vec3(0, 0, 0), length, angle, iterations);
    modelData.vertices = vertices;

    return modelData;
}

void LSystemGenerator::BuildBranch(std::vector<glm::vec3>& vertices, glm::vec3 pos, float length, float angle, int depth) {
    if (depth <= 0) return;

    vertices.push_back(pos);
    glm::vec3 dir = glm::vec3(0, 1, 0); // 初始向上
    pos += dir * length;
    vertices.push_back(pos);

    glm::mat4 rot1 = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 0, 1));
    glm::mat4 rot2 = glm::rotate(glm::mat4(1.0f), glm::radians(-angle), glm::vec3(0, 0, 1));
    BuildBranch(vertices, pos, length * 0.7f, angle, depth - 1);
    BuildBranch(vertices, pos, length * 0.7f, angle, depth - 1);
}