#ifndef PROJECT_TREE_H
#define PROJECT_TREE_H

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <imgui.h>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"
#include "ProjectManager/ProjectManager.h"

namespace MyRenderer {

    class ProjectTree {
    public:
        ProjectTree(std::shared_ptr<EventBus> eventBus);
        ~ProjectTree();

        // 更新目录树 UI，在主线程中调用
        void Update();

        // 设置项目数据并构建树结构
        void SetProjectData(const ProjectData& data);

    private:
        // 订阅事件
        void SubscribeToEvents();

        // 处理项目打开事件
        void OnProjectOpened(const Events::ProjectOpenedEvent& event);

        // 处理模型删除事件
        void OnModelDeleted(const Events::ModelDeletedEvent& event);

        // 递归渲染树节点
        void RenderTreeNode(const std::string& modelUUID, const std::map<std::string, std::vector<std::string>>& hierarchy);

        // 处理拖拽操作
        void HandleDragDrop(const std::string& modelUUID);

        // 处理右键菜单
        void HandleContextMenu(const std::string& modelUUID);

        std::shared_ptr<EventBus> eventBus_;  // 事件总线
        ProjectData projectData_;             // 当前项目数据
        std::map<std::string, ModelData> modelMap_;  // UUID 到模型数据的映射
        std::map<std::string, std::vector<std::string>> hierarchy_;  // 父UUID到子UUID的映射
        std::string selectedModelUUID_;       // 当前选中的模型UUID
    };

} // namespace MyRenderer

#endif // PROJECT_TREE_H