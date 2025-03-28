#pragma once
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "EventBus/EventBus.h"
#include "EventBus/EventTypes.h"

class IProceduralGenerator {
public:
    virtual ~IProceduralGenerator() = default;

    /**
     * @brief 执行程序化生成任务。
     * @param params 生成参数（JSON 格式）。
     * @param eventBus 事件总线，用于发布生成相关事件。
     */
    virtual void Generate(const nlohmann::json& params, std::shared_ptr<EventBus> eventBus) = 0;

    /**
     * @brief 取消当前生成任务。
     */
    virtual void Cancel() = 0;

    /**
     * @brief 获取算法名称。
     * @return 算法的唯一名称。
     */
    virtual std::string GetName() const = 0;
};