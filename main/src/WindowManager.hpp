/**

    @file WindowManager.hpp
    */
    #ifndef WINDOW_MANAGER_HPP
    #define WINDOW_MANAGER_HPP

#include <SDL2/SDL.h>
#include "AIManager.hpp"
#include <string>
#include <vector>
#include <optional>
#include "Live2DManager.hpp"

// 前向声明 ImGuiIO，避免在头文件中包含 imgui.h
struct ImGuiIO;

class WindowManager {
public:
WindowManager(const std::string& title, int width, int height, AIManager& aiManager);
~WindowManager();
bool init();

// 返回一个 optional<string>，如果用户按回车提交了输入则包含该字符串
std::optional<std::string> handleEvents();

void update();
void render();
void getWindowSize(int& width, int& height) const;

// 添加一条新消息到聊天记录
void addMessage(const std::string& author, const std::string& text);

// 获取主循环状态
bool isRunning() const;
private:
void cleanup();
AIManager& aiManager; // 存储引用
Live2DManager _live2dManager;
std::string windowTitle;
int windowWidth;
int windowHeight;
bool running;

SDL_Window* window;
SDL_GLContext glContext; // 不再使用 SDL_Renderer，而是 OpenGL 上下文
ImGuiIO* imgui_io;       // 指向 ImGuiIO 的指针

// UI 状态相关的成员变量保持不变
char currentInputBuffer[256] = {0}; // ImGui 使用 C-style 字符数组更方便
struct ChatMessage {
    std::string author;
    std::string text;
};
std::vector<ChatMessage> chatHistory;
};

#endif // WINDOW_MANAGER_HPP