#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "MaterialManager/MaterialManager.h"
#include "TextureManager/TextureManager.h"

namespace MyRenderer {
    class ControlPanel {
    public:
        // 构造函数，初始化事件总线、材质管理和纹理管理
        ControlPanel(std::shared_ptr<EventBus> eventBus,
                     std::shared_ptr<MaterialManager> materialManager,
                     std::shared_ptr<TextureManager> textureManager);
        // 默认析构函数
        ~ControlPanel() = default;

        // 更新控制面板 UI
        void Update();
        // 设置当前选中的模型
        void SetSelectedModel(const ModelData& model);

    private:
        // 订阅事件
        void SubscribeToEvents();
        // 渲染主 UI
        void RenderUI();
        // 渲染模型变换控件
        void RenderModelTransform();
        // 渲染材质属性控件
        void RenderMaterialProperties();
        // 渲染着色器信息
        void RenderShaderInfo();
        // 渲染纹理选择器
        void RenderTextureSelector();
        // 新增：渲染场景设置（包括光照控制）
        void RenderSceneSettings();

        // 事件处理函数
        void OnModelSelectionChanged(const MyRenderer::Events::ModelSelectionChangedEvent& event); // 模型选择变更
        void OnOperationModeChanged(const MyRenderer::Events::OperationModeChangedEvent& event);   // 操作模式变更
        void OnProjectLoadFailed(const MyRenderer::Events::ProjectLoadFailedEvent& event);         // 项目加载失败
        void OnModelDeleted(const MyRenderer::Events::ModelDeletedEvent& event);                   // 模型删除
        void OnMaterialUpdated(const MyRenderer::Events::MaterialUpdatedEvent& event);             // 材质更新
        // void OnShaderCompiled(const MyRenderer::Events::ShaderCompiledEvent& event);            // 着色器编译（已注释）
        void OnTextureLoaded(const MyRenderer::Events::TextureLoadedEvent& event);                 // 纹理加载

        std::shared_ptr<EventBus> eventBus_;             // 事件总线
        std::shared_ptr<MaterialManager> materialManager_; // 材质管理器
        std::shared_ptr<TextureManager> textureManager_;   // 纹理管理器

        ModelData currentModel_;           // 当前选中模型数据
        MaterialData currentMaterial_;     // 当前材质数据
        std::string currentTexturePath_;   // 当前纹理路径（用于撤销操作）
        bool hasSelectedModel_ = false;    // 是否有选中模型
        std::string shaderCompileStatus_;  // 着色器编译状态
        bool isAnimationPlaying_ = false;  // 动画播放状态锁
    };
} // namespace MyRenderer

#endif // CONTROL_PANEL_H