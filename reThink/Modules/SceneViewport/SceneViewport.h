#ifndef SCENE_VIEWPORT_H
#define SCENE_VIEWPORT_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "ShaderManager/ShaderManager.h"
#include "ModelLoader/ModelLoader.h"
#include "MaterialManager/MaterialManager.h"
#include "TextureManager/TextureManager.h"

namespace MyRenderer {
    enum class OperationMode { Vertex, Edge, Face, Object };
}

class SceneViewport {
public:
    SceneViewport(std::shared_ptr<EventBus> eventBus,
                      std::shared_ptr<ShaderManager> shaderManager,
                      std::shared_ptr<ModelLoader> modelLoader,
                      std::shared_ptr<MaterialManager> materialManager,
                      std::shared_ptr<TextureManager> textureManager,
                      GLFWwindow* window);
    ~SceneViewport();

    void Initialize();
    void Update();
    void Shutdown();

    void SetOperationMode(MyRenderer::OperationMode mode);
    void TransformModel(const std::string& modelUUID, const glm::mat4& transform);
    void LoadDefaultCube();

private:
    void SubscribeToEvents();
    void RenderScene();
    void DrawModel(const ModelData& model);
    void HandleImGuizmo();
    void UpdateAnimationFrame(float currentTime);
    void ApplyShaderChanges(const std::string& vertexPath, const std::string& fragmentPath, bool success);
    void UpdateMaterial(const std::string& materialUUID, const glm::vec3& diffuseColor, const glm::vec3& specularColor, float shininess, const std::string& textureUUID);
    void RemoveTexture(const std::string& textureUUID);
    void AdjustFrameRate(bool isPlaying);
    std::vector<glm::vec3> GenerateGridAndAxesVertices(); // 生成网格和坐标轴顶点

    // 事件处理函数
    void OnModelLoaded(const MyRenderer::Events::ModelLoadedEvent& event);
    void OnModelDeleted(const MyRenderer::Events::ModelDeletedEvent& event);
    void OnOperationModeChanged(const MyRenderer::Events::OperationModeChangedEvent& event);
    void OnAnimationFrameChanged(const MyRenderer::Events::AnimationFrameChangedEvent& event);
    void OnModelTransformed(const MyRenderer::Events::ModelTransformedEvent& event);
    void OnMaterialUpdated(const MyRenderer::Events::MaterialUpdatedEvent& event);
    void OnTextureDeleted(const MyRenderer::Events::TextureDeletedEvent& event);
    void OnViewportFocus(const MyRenderer::Events::ViewportFocusEvent& event);
    void OnAnimationPlaybackStarted(const MyRenderer::Events::AnimationPlaybackStartedEvent& event);
    void OnAnimationPlaybackStopped(const MyRenderer::Events::AnimationPlaybackStoppedEvent& event);
    void OnAnimationUpdated(const MyRenderer::Events::AnimationUpdatedEvent& event);
    void OnHierarchyUpdate(const MyRenderer::Events::HierarchyUpdateEvent& event);
    void OnSceneLightUpdated(const MyRenderer::Events::SceneLightUpdatedEvent& event); // 处理光照更新事件

    std::shared_ptr<EventBus> eventBus_;
    std::shared_ptr<ShaderManager> shaderManager_;
    std::shared_ptr<ModelLoader> modelLoader_;
    std::shared_ptr<MaterialManager> materialManager_;
    std::shared_ptr<TextureManager> textureManager_;
    GLFWwindow* window_;

    std::map<std::string, ModelData> models_; // 场景中的模型
    std::string selectedModelUUID_; // 当前选中的模型 UUID
    MyRenderer::OperationMode currentMode_ = MyRenderer::OperationMode::Object;
    bool isPlaying_ = false; // 动画播放状态
    bool isFocused_ = false; // 视口聚焦状态
    std::map<float, KeyframeData> keyframes_; // 动画关键帧数据

    // OpenGL 资源
    std::map<std::string, GLuint> vaoMap_; // 每个模型的 VAO
    std::map<std::string, GLuint> vboMap_; // 每个模型的 VBO
    std::map<std::string, GLuint> eboMap_; // 每个模型的 EBO
    std::map<std::string, GLuint> normalVboMap_; // 存储法线 VBO

    // 相机参数
    glm::mat4 view_ = glm::mat4(1.0f);
    glm::mat4 projection_ = glm::mat4(1.0f);
    glm::vec3 cameraPos_ = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 cameraFront_ = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp_ = glm::vec3(0.0f, 1.0f, 0.0f);

    // FBO 相关成员
    GLuint fbo_ = 0; // Framebuffer Object 的句柄
    GLuint texture_ = 0; // 渲染目标纹理
    GLuint rbo_ = 0; // 深度缓冲区的渲染缓冲对象
    int fboWidth_ = 0; // FBO 宽度
    int fboHeight_ = 0; // FBO 高度

    // 光照相关
    glm::vec3 lightDir_ = glm::vec3(0.0f, -1.0f, -1.0f); // 默认光源方向
    glm::vec3 lightColor_ = glm::vec3(1.0f, 1.0f, 1.0f); // 默认光源颜色（白色）

    // 网格和坐标轴相关
    GLuint gridAxesVao_ = 0; // 网格和坐标轴的 VAO
    GLuint gridAxesVbo_ = 0; // 网格和坐标轴的 VBO
    unsigned int gridVerticesCount_ = 0; // 网格顶点数
    unsigned int axesVerticesStartIndex_ = 0; // 坐标轴顶点起始索引
    
};

#endif // SCENE_VIEWPORT_H