#include "WindowManager.hpp"
#include "AIManager.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    // 1. 初始化窗口管理器
    WindowManager app("AI Desktop Pet - Step 3", 800, 600);
    if (!app.init()) {
        std::cerr << "Failed to initialize the WindowManager." << std::endl;
        return -1;
    }

    // 2. 初始化 AI 管理器
    const char* apiKey = std::getenv("GOOGLE_AI_STUDIO_API_KEY");
    if (!apiKey) {
        std::cerr << "Error: GOOGLE_AI_STUDIO_API_KEY environment variable not set." << std::endl;
        return 1;
    }
    AIManager aiManager(apiKey);

    // 3. 主应用循环
    while (app.isRunning()) {
        // --- 处理用户输入 ---
        std::optional<std::string> userInput = app.handleEvents();
        if (userInput.has_value()) {
            if (!aiManager.isBusy()) {
                aiManager.sendMessage(userInput.value());
            } else {
                app.addMessage("System", "AI is still thinking, please wait...");
            }
        }

        // --- 检查 AI 回复 ---
        std::string aiResult = aiManager.getResult();
        if (!aiResult.empty()) {
            app.addMessage("AI", aiResult);
        }

        // --- 更新与渲染 ---
        app.update();
        app.render();
        
        // 短暂休眠，避免 CPU 占用率 100%
        SDL_Delay(16); // 大约 60 FPS
    }

    return 0;
}