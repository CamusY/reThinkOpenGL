#include "SceneViewport/SceneViewport.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <ImGuizmo.h>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

SceneViewport::SceneViewport(std::shared_ptr<EventBus> eventBus,
                             std::shared_ptr<ShaderManager> shaderManager,
                             std::shared_ptr<ModelLoader> modelLoader,
                             std::shared_ptr<MaterialManager> materialManager,
                             std::shared_ptr<TextureManager> textureManager,
                             GLFWwindow* window)
    : eventBus_(eventBus),
      shaderManager_(shaderManager),
      modelLoader_(modelLoader),
      materialManager_(materialManager),
      textureManager_(textureManager),
      window_(window) {
    if (!eventBus_ || !shaderManager_ || !modelLoader_ || !materialManager_ || !textureManager_ || !window_) {
        throw std::runtime_error("SceneViewport: 无效的依赖项。");
    }
}

SceneViewport::~SceneViewport() {
    Shutdown();
}

void SceneViewport::Initialize() {
    SubscribeToEvents();

    // 启用深度测试，确保正确渲染
    glEnable(GL_DEPTH_TEST);

    // 初始化 ImGuizmo
    ImGuizmo::SetOrthographic(false);

    // 加载默认立方体模型
    LoadDefaultCube();

    // 初始化网格和坐标轴
    std::vector<glm::vec3> gridAndAxesVertices = GenerateGridAndAxesVertices();
    glGenVertexArrays(1, &gridAxesVao_);
    glBindVertexArray(gridAxesVao_);
    glGenBuffers(1, &gridAxesVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, gridAxesVbo_);
    glBufferData(GL_ARRAY_BUFFER, gridAndAxesVertices.size() * sizeof(glm::vec3), gridAndAxesVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    gridVerticesCount_ = 84; // 网格顶点数 (42 条线，每条线 2 个顶点)
    axesVerticesStartIndex_ = 84; // 坐标轴顶点起始索引 (3 条轴，每条 2 个顶点)
}

void SceneViewport::Update() {
    // 创建 ImGui 窗口
    ImGui::Begin("SceneViewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // 获取 ImGui 窗口的可用内容区域大小
    ImVec2 size = ImGui::GetContentRegionAvail();
    int width = static_cast<int>(size.x);
    int height = static_cast<int>(size.y);

    // 检查 FBO 大小是否需要更新
    if (width != fboWidth_ || height != fboHeight_) {
        if (fbo_ != 0) {
            glDeleteFramebuffers(1, &fbo_);
            glDeleteTextures(1, &texture_);
            glDeleteRenderbuffers(1, &rbo_);
        }

        // 创建新的 FBO
        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

        // 创建渲染目标纹理
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);

        // 创建深度缓冲区
        glGenRenderbuffers(1, &rbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_);

        // 检查 FBO 是否完整
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[错误] FBO 创建失败！" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        fboWidth_ = width;
        fboHeight_ = height;
    }

    // 绑定 FBO 并渲染场景
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, fboWidth_, fboHeight_);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 设置投影矩阵和视图矩阵
    glEnable(GL_DEPTH_TEST);
    float aspect = static_cast<float>(fboWidth_) / static_cast<float>(fboHeight_);
    projection_ = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    view_ = glm::lookAt(cameraPos_, cameraPos_ + cameraFront_, cameraUp_);
    RenderScene();
    glDisable(GL_DEPTH_TEST);

    // 解绑 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 在 ImGui 中显示渲染结果
    ImGui::Image((ImTextureID)(intptr_t)texture_, ImVec2(fboWidth_, fboHeight_), ImVec2(0, 1), ImVec2(1, 0));

    // 处理 ImGuizmo
    if (!selectedModelUUID_.empty() && currentMode_ == MyRenderer::OperationMode::Object) {
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, size.x, size.y);
        HandleImGuizmo();
    }

    ImGui::End();

    textureManager_->ProcessTextureUploadQueue();
}

void SceneViewport::Shutdown() {
    // 清理 VAO、VBO、EBO
    for (auto& [uuid, vao] : vaoMap_) {
        glDeleteVertexArrays(1, &vao);
    }
    for (auto& [uuid, vbo] : vboMap_) {
        glDeleteBuffers(1, &vbo);
    }
    for (auto& [uuid, vbo] : normalVboMap_) {
        glDeleteBuffers(1, &vbo);
    }
    for (auto& [uuid, ebo] : eboMap_) {
        glDeleteBuffers(1, &ebo);
    }
    vaoMap_.clear();
    vboMap_.clear();
    normalVboMap_.clear();
    eboMap_.clear();

    // 清理网格和坐标轴资源
    if (gridAxesVao_ != 0) {
        glDeleteVertexArrays(1, &gridAxesVao_);
    }
    if (gridAxesVbo_ != 0) {
        glDeleteBuffers(1, &gridAxesVbo_);
    }

    // 清理 FBO 相关资源
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
    }
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
    }
    if (rbo_ != 0) {
        glDeleteRenderbuffers(1, &rbo_);
    }
}

void SceneViewport::SetOperationMode(MyRenderer::OperationMode mode) {
    currentMode_ = mode;
}

void SceneViewport::TransformModel(const std::string& modelUUID, const glm::mat4& transform) {
    auto it = models_.find(modelUUID);
    if (it != models_.end()) {
        it->second.transform = transform;
    }
}

void SceneViewport::LoadDefaultCube() {
    // 创建一个默认立方体模型
    ModelData cube;
    cube.uuid = "default_cube";
    cube.filepath = "internal:cube";
    cube.transform = glm::mat4(1.0f); // 放置在世界中心
    cube.vertexShaderPath = "Shaders/default.vs";
    cube.fragmentShaderPath = "Shaders/default.fs";
    cube.materialUUIDs.push_back("default_material");

    // 定义立方体的顶点数据
    cube.vertices = {
        // 前
        {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        // 后
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
        // 左
        {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f},
        // 右
        { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
        // 上
        {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        // 下
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}
    };

    // 定义立方体的法线数据
    cube.normals = {
        // 前
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        // 后
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
        // 左
        {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        // 右
        {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        // 上
        {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        // 下
        {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}
    };

    // 定义立方体的索引数据
    cube.indices = {
        0, 1, 2, 2, 3, 0,    // 前
        4, 5, 6, 6, 7, 4,    // 后
        8, 9, 10, 10, 11, 8, // 左
        12, 13, 14, 14, 15, 12, // 右
        16, 17, 18, 18, 19, 16, // 上
        20, 21, 22, 22, 23, 20  // 下
    };

    // 将立方体添加到模型列表
    models_[cube.uuid] = cube;
    eventBus_->Publish(MyRenderer::Events::ModelLoadedEvent{cube});
}

void SceneViewport::SubscribeToEvents() {
    eventBus_->Subscribe<MyRenderer::Events::ModelLoadedEvent>(
        [this](const auto& event) { OnModelLoaded(event); });
    eventBus_->Subscribe<MyRenderer::Events::ModelDeletedEvent>(
        [this](const auto& event) { OnModelDeleted(event); });
    eventBus_->Subscribe<MyRenderer::Events::OperationModeChangedEvent>(
        [this](const auto& event) { OnOperationModeChanged(event); });
    eventBus_->Subscribe<MyRenderer::Events::AnimationFrameChangedEvent>(
        [this](const auto& event) { OnAnimationFrameChanged(event); });
    eventBus_->Subscribe<MyRenderer::Events::ModelTransformedEvent>(
        [this](const auto& event) { OnModelTransformed(event); });
    eventBus_->Subscribe<MyRenderer::Events::MaterialUpdatedEvent>(
        [this](const auto& event) { OnMaterialUpdated(event); });
    eventBus_->Subscribe<MyRenderer::Events::TextureDeletedEvent>(
        [this](const auto& event) { OnTextureDeleted(event); });
    eventBus_->Subscribe<MyRenderer::Events::ViewportFocusEvent>(
        [this](const auto& event) { OnViewportFocus(event); });
    eventBus_->Subscribe<MyRenderer::Events::AnimationPlaybackStartedEvent>(
        [this](const auto& event) { OnAnimationPlaybackStarted(event); });
    eventBus_->Subscribe<MyRenderer::Events::AnimationPlaybackStoppedEvent>(
        [this](const auto& event) { OnAnimationPlaybackStopped(event); });
    eventBus_->Subscribe<MyRenderer::Events::AnimationUpdatedEvent>(
        [this](const auto& event) { OnAnimationUpdated(event); });
    eventBus_->Subscribe<MyRenderer::Events::HierarchyUpdateEvent>(
        [this](const auto& event) { OnHierarchyUpdate(event); });
    eventBus_->Subscribe<MyRenderer::Events::SceneLightUpdatedEvent>(
        [this](const auto& event) { OnSceneLightUpdated(event); });
}

void SceneViewport::RenderScene() {
    // 渲染网格和坐标轴（关闭深度测试以确保始终可见）
    glDisable(GL_DEPTH_TEST);
    GLuint lineProgram = shaderManager_->GetShaderProgram(
        "Shaders/default_line.vs",
        "Shaders/default_line.fs"
    );
    if (lineProgram != 0) {
        glUseProgram(lineProgram);
        glUniformMatrix4fv(glGetUniformLocation(lineProgram, "model"), 1, GL_FALSE, &glm::mat4(1.0f)[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lineProgram, "view"), 1, GL_FALSE, &view_[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lineProgram, "projection"), 1, GL_FALSE, &projection_[0][0]);
        glBindVertexArray(gridAxesVao_);
        // 绘制网格（灰色）
        glUniform3f(glGetUniformLocation(lineProgram, "color"), 0.5f, 0.5f, 0.5f);
        glDrawArrays(GL_LINES, 0, gridVerticesCount_);
        // 绘制 X 轴（红色）
        glUniform3f(glGetUniformLocation(lineProgram, "color"), 1.0f, 0.0f, 0.0f);
        glDrawArrays(GL_LINES, axesVerticesStartIndex_, 2);
        // 绘制 Y 轴（绿色）
        glUniform3f(glGetUniformLocation(lineProgram, "color"), 0.0f, 1.0f, 0.0f);
        glDrawArrays(GL_LINES, axesVerticesStartIndex_ + 2, 2);
        // 绘制 Z 轴（蓝色）
        glUniform3f(glGetUniformLocation(lineProgram, "color"), 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINES, axesVerticesStartIndex_ + 4, 2);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    // 渲染模型（启用深度测试）
    glEnable(GL_DEPTH_TEST);
    for (const auto& [uuid, model] : models_) {
        DrawModel(model);
    }
}

void SceneViewport::DrawModel(const ModelData& model) {
    // 设置着色器程序
    GLuint program = shaderManager_->GetShaderProgram(model.vertexShaderPath, model.fragmentShaderPath);
    if (program == 0) {
        std::cerr << "[错误] 着色器程序未加载: " << model.vertexShaderPath << " | " << model.fragmentShaderPath << std::endl;
        return;
    }
    glUseProgram(program);

    // 绑定材质
    if (!model.materialUUIDs.empty()) {
        materialManager_->BindMaterial(model.materialUUIDs[0]);
    }

    // 设置 MVP 矩阵
    glm::mat4 modelMatrix = model.transform;
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view_));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection_));

    // 设置光照参数
    glUniform3fv(glGetUniformLocation(program, "lightDir"), 1, glm::value_ptr(lightDir_));
    glUniform3fv(glGetUniformLocation(program, "lightColor"), 1, glm::value_ptr(lightColor_));
    glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(cameraPos_));

    // 绑定 VAO
    auto vaoIt = vaoMap_.find(model.uuid);
    if (vaoIt == vaoMap_.end()) {
        // 初始化 VAO、VBO、EBO
        GLuint vao, vbo, normalVbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &normalVbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        // 顶点位置
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(glm::vec3), model.vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        // 法线
        glBindBuffer(GL_ARRAY_BUFFER, normalVbo);
        glBufferData(GL_ARRAY_BUFFER, model.normals.size() * sizeof(glm::vec3), model.normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(1);
        // 索引
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);

        vaoMap_[model.uuid] = vao;
        vboMap_[model.uuid] = vbo;
        normalVboMap_[model.uuid] = normalVbo;
        eboMap_[model.uuid] = ebo;
    }

    glBindVertexArray(vaoMap_[model.uuid]);

    // 高亮显示选中模型
    if (model.uuid == selectedModelUUID_) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
        glUniform3f(glGetUniformLocation(program, "outlineColor"), 0.0f, 1.0f, 1.0f); // 青色 #00FFFF
        glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // 正常渲染
    glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, 0);

    // 编辑模式下的高亮
    if (model.uuid == selectedModelUUID_ && currentMode_ != MyRenderer::OperationMode::Object) {
        glPointSize(8.0f);
        glLineWidth(3.0f);
        switch (currentMode_) {
            case MyRenderer::OperationMode::Vertex:
                glUniform3f(glGetUniformLocation(program, "highlightColor"), 1.0f, 0.0f, 0.0f); // 红色 #FF0000
                glDrawArrays(GL_POINTS, 0, model.vertices.size());
                break;
            case MyRenderer::OperationMode::Edge:
                glUniform3f(glGetUniformLocation(program, "highlightColor"), 1.0f, 1.0f, 0.0f); // 黄色 #FFFF00
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, 0);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            case MyRenderer::OperationMode::Face:
                // 面模式暂不实现额外高亮，仅正常渲染
                break;
            default:
                break;
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void SceneViewport::HandleImGuizmo() {
    auto it = models_.find(selectedModelUUID_);
    if (it == models_.end()) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    glm::mat4 oldTransform = it->second.transform;
    glm::mat4 newTransform = oldTransform;

    if (ImGuizmo::IsUsing()) {
        ImGuizmo::Manipulate(glm::value_ptr(view_), glm::value_ptr(projection_),
                             ImGuizmo::TRANSLATE, ImGuizmo::WORLD,
                             glm::value_ptr(newTransform));

        if (newTransform != oldTransform) {
            Operation op;
            op.execute = [this, uuid = selectedModelUUID_, newTransform]() {
                eventBus_->Publish(MyRenderer::Events::ModelTransformedEvent{uuid, newTransform});
            };
            op.undo = [this, uuid = selectedModelUUID_, oldTransform]() {
                eventBus_->Publish(MyRenderer::Events::ModelTransformedEvent{uuid, oldTransform});
            };
            eventBus_->Publish(MyRenderer::Events::PushUndoOperationEvent{op});
            eventBus_->Publish(MyRenderer::Events::ModelTransformedEvent{selectedModelUUID_, newTransform});
        }
    }
}

void SceneViewport::UpdateAnimationFrame(float currentTime) {
    for (const auto& [time, keyframe] : keyframes_) {
        if (std::abs(time - currentTime) < 0.01f) { // 假设时间精度为 0.01s
            auto it = models_.find(keyframe.modelUUID);
            if (it != models_.end()) {
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), keyframe.position) *
                                      glm::mat4_cast(keyframe.rotation) *
                                      glm::scale(glm::mat4(1.0f), keyframe.scale);
                it->second.transform = transform;
            }
        }
    }
}

void SceneViewport::ApplyShaderChanges(const std::string& vertexPath, const std::string& fragmentPath, bool success) {
    if (success) {
        // ShaderManager 已更新缓存，下次渲染自动使用新着色器
    } else {
        for (auto& [uuid, model] : models_) {
            if (model.vertexShaderPath == vertexPath && model.fragmentShaderPath == fragmentPath) {
                model.vertexShaderPath = "Shaders/default.vs";
                model.fragmentShaderPath = "Shaders/default.fs";
                eventBus_->Publish(MyRenderer::Events::ModelTransformedEvent{uuid, model.transform});
            }
        }
    }
}

void SceneViewport::UpdateMaterial(const std::string& materialUUID, const glm::vec3& diffuseColor, const glm::vec3& specularColor, float shininess, const std::string& textureUUID) {
    auto material = materialManager_->GetMaterial(materialUUID);
    if (material) {
        material->SetDiffuseColor(diffuseColor);
        material->SetSpecularColor(specularColor);
        material->SetShininess(shininess);
        material->SetTextureUUID(textureUUID);
    }
}

void SceneViewport::RemoveTexture(const std::string& textureUUID) {
    for (auto& [uuid, model] : models_) {
        for (const auto& materialUUID : model.materialUUIDs) {
            auto material = materialManager_->GetMaterial(materialUUID);
            if (material && material->GetTextureUUID() == textureUUID) {
                material->SetTextureUUID("");
                eventBus_->Publish(MyRenderer::Events::MaterialUpdatedEvent{
                    materialUUID,
                    material->GetDiffuseColor(),
                    material->GetSpecularColor(),
                    material->GetShininess(),
                    ""
                });
            }
        }
    }
}

void SceneViewport::AdjustFrameRate(bool isPlaying) {
    if (isPlaying) {
        glfwSwapInterval(0); // 关闭 V-Sync，允许高帧率
    } else {
        glfwSwapInterval(1); // 启用 V-Sync，限制帧率
    }
}

std::vector<glm::vec3> SceneViewport::GenerateGridAndAxesVertices() {
    std::vector<glm::vec3> vertices;
    // 网格：水平线
    for (int y = -10; y <= 10; ++y) {
        vertices.push_back({-10.0f, static_cast<float>(y), 0.0f});
        vertices.push_back({10.0f, static_cast<float>(y), 0.0f});
    }
    // 网格：垂直线
    for (int x = -10; x <= 10; ++x) {
        vertices.push_back({static_cast<float>(x), -10.0f, 0.0f});
        vertices.push_back({static_cast<float>(x), 10.0f, 0.0f});
    }
    // 坐标轴
    // X 轴：红色
    vertices.push_back({0.0f, 0.0f, 0.0f});
    vertices.push_back({10.0f, 0.0f, 0.0f});
    // Y 轴：绿色
    vertices.push_back({0.0f, 0.0f, 0.0f});
    vertices.push_back({0.0f, 10.0f, 0.0f});
    // Z 轴：蓝色
    vertices.push_back({0.0f, 0.0f, 0.0f});
    vertices.push_back({0.0f, 0.0f, -10.0f});
    return vertices;
}

void SceneViewport::OnModelLoaded(const MyRenderer::Events::ModelLoadedEvent& event) {
    models_[event.modelData.uuid] = event.modelData;
}

void SceneViewport::OnModelDeleted(const MyRenderer::Events::ModelDeletedEvent& event) {
    auto it = models_.find(event.modelUUID);
    if (it != models_.end()) {
        glDeleteVertexArrays(1, &vaoMap_[event.modelUUID]);
        glDeleteBuffers(1, &vboMap_[event.modelUUID]);
        glDeleteBuffers(1, &normalVboMap_[event.modelUUID]);
        glDeleteBuffers(1, &eboMap_[event.modelUUID]);
        vaoMap_.erase(event.modelUUID);
        vboMap_.erase(event.modelUUID);
        normalVboMap_.erase(event.modelUUID);
        eboMap_.erase(event.modelUUID);
        models_.erase(it);
        if (selectedModelUUID_ == event.modelUUID) {
            selectedModelUUID_.clear();
        }
    }
}

void SceneViewport::OnOperationModeChanged(const MyRenderer::Events::OperationModeChangedEvent& event) {
    switch (event.mode) {
    case MyRenderer::Events::OperationModeChangedEvent::Mode::Vertex:
        currentMode_ = MyRenderer::OperationMode::Vertex;
        break;
    case MyRenderer::Events::OperationModeChangedEvent::Mode::Edge:
        currentMode_ = MyRenderer::OperationMode::Edge;
        break;
    case MyRenderer::Events::OperationModeChangedEvent::Mode::Face:
        currentMode_ = MyRenderer::OperationMode::Face;
        break;
    case MyRenderer::Events::OperationModeChangedEvent::Mode::Object:
        currentMode_ = MyRenderer::OperationMode::Object;
        break;
    }
}

void SceneViewport::OnAnimationFrameChanged(const MyRenderer::Events::AnimationFrameChangedEvent& event) {
    if (isPlaying_) {
        UpdateAnimationFrame(event.currentTime);
    }
}

void SceneViewport::OnModelTransformed(const MyRenderer::Events::ModelTransformedEvent& event) {
    TransformModel(event.modelUUID, event.transform);
}

void SceneViewport::OnMaterialUpdated(const MyRenderer::Events::MaterialUpdatedEvent& event) {
    UpdateMaterial(event.materialUUID, event.diffuseColor, event.specularColor, event.shininess, event.textureUUID);
}

void SceneViewport::OnTextureDeleted(const MyRenderer::Events::TextureDeletedEvent& event) {
    RemoveTexture(event.textureUUID);
}

void SceneViewport::OnViewportFocus(const MyRenderer::Events::ViewportFocusEvent& event) {
    isFocused_ = event.focusState;
    eventBus_->Publish(MyRenderer::Events::ViewportFocusEvent{isFocused_});
}

void SceneViewport::OnAnimationPlaybackStarted(const MyRenderer::Events::AnimationPlaybackStartedEvent& event) {
    isPlaying_ = true;
    AdjustFrameRate(true);
}

void SceneViewport::OnAnimationPlaybackStopped(const MyRenderer::Events::AnimationPlaybackStoppedEvent& event) {
    isPlaying_ = false;
    AdjustFrameRate(false);
}

void SceneViewport::OnAnimationUpdated(const MyRenderer::Events::AnimationUpdatedEvent& event) {
    keyframes_ = event.keyframes;
}

void SceneViewport::OnHierarchyUpdate(const MyRenderer::Events::HierarchyUpdateEvent& event) {
    for (auto& [uuid, model] : models_) {
        if (model.parentUUID == event.parentUUID) {
            model.transform = event.transform * model.transform;
        }
    }
}

void SceneViewport::OnSceneLightUpdated(const MyRenderer::Events::SceneLightUpdatedEvent& event) {
    lightDir_ = event.lightDir;
    lightColor_ = event.lightColor;
}