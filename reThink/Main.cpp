#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include "Modules/Window/Window.h"

using namespace MyRenderer;

int main(int argc, char** argv) {
    system("chcp 65001");
    std::cout << "当前工作目录: " << std::filesystem::current_path() << std::endl;
    try {
        std::cout << "=== 应用程序启动 ===" << std::endl;
        
        // 初始化事件总线
        std::cout << "[主程序] 正在创建事件总线..." << std::endl;
        auto eventBus = std::make_shared<EventBus>();
        
        // 创建主窗口
        std::cout << "[主程序] 正在创建主窗口..." << std::endl;
        auto window = std::make_shared<Window>(eventBus);
        
        // 初始化窗口
        std::cout << "[主程序] 正在初始化窗口..." << std::endl;
        window->Initialize();
        
        // 主循环
        std::cout << "[主程序] 进入主循环..." << std::endl;
        while (!window->ShouldClose()) {
            window->Update();
        }
        
        // 清理资源
        std::cout << "[主程序] 正在关闭应用程序..." << std::endl;
        window->Shutdown();
    } catch (const std::exception& e) {
        std::cerr << "[崩溃] 程序异常: " << e.what() << std::endl;
        return -1;
    }
    
    std::cout << "=== 应用程序正常退出 ===" << std::endl;
    return 0;
}