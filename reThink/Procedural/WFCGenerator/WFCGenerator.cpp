#include "WFCGenerator.h"
#include <chrono>
#include <random>
#include <future>
#include "Utils/MathUtils.h"

WFCGenerator::WFCGenerator(std::shared_ptr<ModelLoader> modelLoader) 
    : modelLoader_(modelLoader), cancelled_(false) {
    if (!modelLoader_) throw std::invalid_argument("WFCGenerator: ModelLoader cannot be null");
}

WFCGenerator::~WFCGenerator() {
    if (generationThread_.joinable()) {
        Cancel();
        generationThread_.join();
    }
}

void WFCGenerator::Generate(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus) {
    cancelled_ = false;
    if (generationThread_.joinable()) {
        generationThread_.join();
    }
    generationThread_ = std::thread(&WFCGenerator::RunGeneration, this, params, eventBus);
}

void WFCGenerator::Cancel() {
    cancelled_ = true;
}

void WFCGenerator::RunGeneration(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus) {
    eventBus->Publish(MyRenderer::Events::ProceduralGenerationStartedEvent{});

    std::vector<std::string> tileSet = params.value("tileSet", std::vector<std::string>{});
    int width = params.value("width", 10);
    int height = params.value("height", 10);
    int depth = params.value("depth", 10);
    nlohmann::json adjacencyRules = params.value("adjacencyRules", nlohmann::json::object());

    if (tileSet.empty()) {
        eventBus->Publish(MyRenderer::Events::ProceduralGenerationCompletedEvent{false, "No tiles provided", ModelData{}});
        return;
    }

    ModelData modelData = GenerateGrid(tileSet, adjacencyRules, width, height, depth);

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

ModelData WFCGenerator::GenerateGrid(const std::vector<std::string>& tileSet, const nlohmann::json& adjacencyRules, int width, int height, int depth) {
    ModelData modelData;
    modelData.uuid = "wfc_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    modelData.filepath = "";
    modelData.transform = glm::mat4(1.0f);
    modelData.vertexShaderPath = "";
    modelData.fragmentShaderPath = "";
    modelData.parentUUID = "";

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> tileDist(0, tileSet.size() - 1);

    // 初始化网格，所有单元格初始包含所有可能瓦片
    std::vector<std::vector<std::vector<Cell>>> grid(height, std::vector<std::vector<Cell>>(width, std::vector<Cell>(depth)));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int z = 0; z < depth; z++) {
                for (int i = 0; i < tileSet.size(); i++) {
                    grid[y][x][z].possibleTiles.push_back(i);
                }
            }
        }
    }

    // 从随机位置开始坍缩
    int startX = width / 2, startY = height / 2, startZ = depth / 2;
    Collapse(grid, startX, startY, startZ, tileSet, adjacencyRules);
    Propagate(grid, startX, startY, startZ, tileSet, adjacencyRules);

    // 生成模型数据
    std::map<std::string, ModelData> loadedTiles;
    for (const auto& tilePath : tileSet) {
        auto future = modelLoader_->LoadModelAsync(tilePath);
        loadedTiles[tilePath] = future.get();
    }

    for (int y = 0; y < height && !cancelled_; y++) {
        for (int x = 0; x < width && !cancelled_; x++) {
            for (int z = 0; z < depth && !cancelled_; z++) {
                if (grid[y][x][z].collapsed && !grid[y][x][z].possibleTiles.empty()) {
                    int tileIdx = grid[y][x][z].possibleTiles[0];
                    const ModelData& tileData = loadedTiles[tileSet[tileIdx]];
                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
                    for (const auto& vertex : tileData.vertices) {
                        glm::vec4 transformed = transform * glm::vec4(vertex, 1.0f);
                        modelData.vertices.push_back(glm::vec3(transformed));
                    }
                    int baseIdx = modelData.vertices.size() - tileData.vertices.size();
                    for (const auto& idx : tileData.indices) {
                        modelData.indices.push_back(baseIdx + idx);
                    }
                }
            }
        }
    }

    return modelData;
}

void WFCGenerator::Collapse(std::vector<std::vector<std::vector<Cell>>>& grid, int x, int y, int z, const std::vector<std::string>& tileSet, const nlohmann::json& adjacencyRules) {
    if (x < 0 || x >= grid[0].size() || y < 0 || y >= grid.size() || z < 0 || z >= grid[0][0].size() || grid[y][x][z].collapsed) return;

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dist(0, grid[y][x][z].possibleTiles.size() - 1);
    int tileIdx = grid[y][x][z].possibleTiles[dist(gen)];
    grid[y][x][z].possibleTiles = {tileIdx};
    grid[y][x][z].collapsed = true;
}

void WFCGenerator::Propagate(std::vector<std::vector<std::vector<Cell>>>& grid, int x, int y, int z, const std::vector<std::string>& tileSet, const nlohmann::json& adjacencyRules) {
    std::vector<std::tuple<int, int, int>> stack = {{x, y, z}};
    while (!stack.empty() && !cancelled_) {
        auto [cx, cy, cz] = stack.back();
        stack.pop_back();

        std::vector<std::tuple<int, int, int>> directions = {
            {cx + 1, cy, cz}, {cx - 1, cy, cz}, {cx, cy + 1, cz}, {cx, cy - 1, cz}, {cx, cy, cz + 1}, {cx, cy, cz - 1}
        };

        for (const auto& [nx, ny, nz] : directions) {
            if (nx < 0 || nx >= grid[0].size() || ny < 0 || ny >= grid.size() || nz < 0 || nz >= grid[0][0].size() || grid[ny][nx][nz].collapsed) continue;

            std::vector<int> newPossible;
            for (int tileIdx : grid[ny][nx][nz].possibleTiles) {
                bool valid = true;
                for (int adjIdx : grid[cy][cx][cz].possibleTiles) {
                    std::string tile = tileSet[tileIdx];
                    std::string adjTile = tileSet[adjIdx];
                    auto adjList = adjacencyRules.value(adjTile, nlohmann::json::array());
                    if (std::find(adjList.begin(), adjList.end(), tile) == adjList.end()) {
                        valid = false;
                        break;
                    }
                }
                if (valid) newPossible.push_back(tileIdx);
            }

            if (newPossible.empty()) {
                // 矛盾，需回溯（简化实现中忽略）
                continue;
            }

            if (newPossible.size() != grid[ny][nx][nz].possibleTiles.size()) {
                grid[ny][nx][nz].possibleTiles = newPossible;
                if (newPossible.size() == 1) {
                    grid[ny][nx][nz].collapsed = true;
                }
                stack.emplace_back(nx, ny, nz);
            }
        }
    }
}