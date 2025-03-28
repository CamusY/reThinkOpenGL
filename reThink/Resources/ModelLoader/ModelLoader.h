#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <string>
#include <map>
#include <memory>
#include <future>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "EventBus/EventBus.h"
#include "ThreadPool/ThreadPool.h"
#include "EventBus/EventTypes.h"
class MaterialManager;

class ModelLoader {
public:
    /**
     * @brief 构造函数，初始化 ModelLoader。
     * @param eventBus 事件总线，用于发布加载和删除事件。
     * @param threadPool 线程池，用于异步加载模型。
     */
    ModelLoader(std::shared_ptr<EventBus> eventBus, std::shared_ptr<ThreadPool> threadPool, std::shared_ptr<MaterialManager> materialManager);

    /**
     * @brief 析构函数，清理资源。
     */
    ~ModelLoader();

    /**
     * @brief 异步加载模型文件。
     * @param filepath 模型文件路径（支持 glTF、OBJ 等格式）。
     * @param priority 任务优先级，默认为 0。
     * @return std::future<ModelData> 返回加载结果的 future 对象。
     */
    std::future<ModelData> LoadModelAsync(const std::string& filepath, int priority = 0);

    /**
     * @brief 删除指定模型并发布删除事件。
     * @param modelUUID 模型的唯一标识符。
     */
    void DeleteModel(const std::string& modelUUID);

    /**
     * @brief 获取指定模型的数据。
     * @param modelUUID 模型的唯一标识符。
     * @return ModelData 模型数据，若不存在则返回空 ModelData。
     */
    ModelData GetModelData(const std::string& modelUUID) const;

private:
    /**
     * @brief 处理 Assimp 加载的场景数据，转换为 ModelData。
     * @param filepath 模型文件路径。
     * @param scene Assimp 加载的场景对象。
     * @return ModelData 转换后的模型数据。
     */
    ModelData ProcessScene(const std::string& filepath, const aiScene* scene);

    /**
     * @brief 处理 Assimp（Asset Importer）库中的一个节点，包括其子节点和网格数据。
     * 
     * @param node 指向 aiNode 对象的指针，表示要处理的节点。
     * @param scene 指向 aiScene 对象的指针，表示整个场景，用于访问节点和网格数据。
     * @param modelData ModelData 对象的引用，用于存储处理后的模型数据。
     * @param parentUUID 父节点的 UUID，用于建立节点之间的层级关系。
     * 
     * 该函数主要用于遍历和处理场景中的节点及其子节点。
     * 它会处理节点的网格数据（如果有），并将相关数据存储到 modelData 对象中。
     * 函数还通过传递 parentUUID 来处理节点之间的层级关系，建立父子关系。
     * 注意：该函数不处理节点的变换属性，例如位置、旋转和缩放。
     */
    void ProcessNode(const aiNode* node, const aiScene* scene, ModelData& modelData, const std::string& parentUUID);

    /**
     * @brief 处理 Assimp 的网格数据。
     * @param mesh Assimp 的网格对象。
     * @param modelData 目标 ModelData 对象，用于存储顶点和索引数据。
     */
    void ProcessMesh(const aiMesh* mesh, ModelData& modelData, const aiScene* scene);

    /**
     * @brief 生成唯一的 UUID。
     * @return std::string 生成的 UUID。
     */
    std::string GenerateUUID() const;

    std::shared_ptr<EventBus> eventBus_;              // 事件总线实例
    std::shared_ptr<ThreadPool> threadPool_;          // 线程池实例
    Assimp::Importer importer_;                       // Assimp 导入器实例
    std::map<std::string, ModelData> loadedModels_; // 已加载模型的缓存
    std::shared_ptr<MaterialManager> materialManager_;    // 材质管理器
    mutable std::mutex mutex_;                        // 互斥锁，确保线程安全
};

#endif // MODEL_LOADER_H