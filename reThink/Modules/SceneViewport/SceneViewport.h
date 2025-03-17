// SceneViewport.h
#pragma once
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "ShaderManager.h"
#include "ModelLoader.h"
#include "Material.h"
#include "ImGuizmo.h"

// 前置声明依赖接口
class ShaderManager;
class ModelLoader;
class Material;

/**
 * @brief 3D场景视口管理模块
 * @note 通过依赖注入获取必要组件，支持多视图模式和编辑工具
 */
class SceneViewport {
public:
    /**
     * @brief 构造函数注入核心依赖项
     * @param shaderManager 着色器管理器（必须）
     * @param modelLoader 模型加载器（必须）
     * @throws std::invalid_argument 任一参数为空指针时抛出
     */
    SceneViewport(
        std::shared_ptr<ShaderManager> shaderManager,
        std::shared_ptr<ModelLoader> modelLoader
    );

    /**
     * @brief 设置材质系统（可选依赖）
     * @param material 材质系统实例
     */
    void SetMaterial(std::shared_ptr<Material> material);

    /**
     * @brief 渲染场景视口
     * @param viewportPos 视口位置
     * @param viewportSize 视口尺寸
     * @param viewMode 视图模式（0-透视，1-正交，2-轴向）
     */
    void Render(const ImVec2& viewportPos, const ImVec2& viewportSize, int viewMode = 0);

    /**
     * @brief 处理视口交互（选择/变换）
     * @param mousePos 鼠标位置（相对视口坐标系）
     */
    void HandleInteraction(const ImVec2& mousePos);

private:
    // 核心依赖项
    std::shared_ptr<ShaderManager> shaderManager_;
    std::shared_ptr<ModelLoader> modelLoader_;
    std::shared_ptr<Material> material_;

    // 渲染状态
    glm::mat4 viewMatrix_;
    glm::mat4 projectionMatrix_;
    unsigned int framebuffer_;
    unsigned int textureColorBuffer_;
    unsigned int rboDepthStencil_;

    // 编辑模式状态
    ImGuizmo::OPERATION currentGizmoOp_ = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode_ = ImGuizmo::WORLD;
    int selectedEntityId_ = -1;

    // 初始化方法
    void InitRenderBuffers();
    void SetupCamera(int viewMode, const ImVec2& viewportSize);
    void ValidateDependencies() const;

    // 渲染子方法
    void RenderMainPass();
    void RenderGizmo(const ImVec2& viewportPos, const ImVec2& viewportSize);
    void DrawGrid() const;
};

// 视口删除器（管理OpenGL资源）
struct FramebufferDeleter {
    void operator()(unsigned int* fb) const {
        if(fb) {
            glDeleteFramebuffers(1, fb);
            delete fb;
        }
    }
};