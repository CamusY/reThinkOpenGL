// EventBus_test.cpp（示例测试代码片段）
#include "EventBus.h"
#include "EventTypes.h"
#include <cassert>
#include <iostream>

void testEventBus() {
    // 创建事件总线（注入简单日志）
    EventBus bus([](const std::string& msg) {
        std::cout << "[TEST] " << msg << std::endl;
    });

    bool eventReceived = false;
    
    // 订阅模型变换事件
    bus.Subscribe<EventTypes::ModelTransformedEvent>(
        [&](const void* data) {
            const auto& event = *static_cast<const EventTypes::ModelTransformedEvent*>(data);
            assert(event.modelUUID == "model-123");
            eventReceived = true;
        });

    // 发布测试事件
    EventTypes::ModelTransformedEvent event;
    event.modelUUID = "model-123";
    bus.Publish(event);

    assert(eventReceived);
}