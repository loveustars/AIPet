/**
 * @file main.cpp
 */
#include "WindowManager.hpp"
#include "AIManager.hpp"
#include <iostream>

// 临时占位函数，用于连接 ImGui 和 AIManager
// 可以把它变成 WindowManager 的一个成员函数，或者用其他方式实现
void handleUserInput(AIManager& aiManager, WindowManager& app, const std::string& userInput) {
    if (!aiManager.isBusy()) {
        aiManager.sendMessage(userInput);
    } else {
        app.addMessage("System", "AI is still thinking, please wait...");
    }
}

int main(int argc, char* argv[]) {
    // 初始化
    // 省略了 API Key 的获取，因为 AIManager 内部已经处理
    AIManager aiManager(std::getenv("GOOGLE_AI_STUDIO_API_KEY") ? std::getenv("GOOGLE_AI_STUDIO_API_KEY") : "");

    WindowManager app("AI Desktop Pet - OpenGL", 800, 600, aiManager); 
    if (!app.init()) {
        std::cerr << "Failed to initialize the WindowManager." << std::endl;
        return -1;
    }

    // 主循环
    while (app.isRunning()) {
        // handleEvents 现在只处理窗口关闭等事件
        app.handleEvents();

        // 检查 AI 是否有新回复
        std::string aiResult = aiManager.getResult();
        if (!aiResult.empty()) {
            app.addMessage("AI", aiResult);
        }

        // 更新与渲染
        app.update();
        app.render(); // render 函数内部现在处理用户输入提交
        
        // SDL_Delay 不再那么必要，因为垂直同步会限制帧率
        // SDL_Delay(5); 
    }

    return 0;
}