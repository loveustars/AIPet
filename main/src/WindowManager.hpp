#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // 包含 TTF 头文件
#include <string>
#include <vector>
#include <optional>

class WindowManager {
public:
    WindowManager(const std::string& title, int width, int height);
    ~WindowManager();

    bool init();
    
    // 主循环现在由 main 函数控制，所以 run() 函数移除
    // void run(); 

    // handleEvents 现在返回一个 optional<string>。
    // 如果用户按了回车，它就包含输入的字符串，否则为空。
    std::optional<std::string> handleEvents();
    
    void update();
    void render();
    void cleanup();

    // 添加一条新消息到聊天记录中
    void addMessage(const std::string& author, const std::string& text);

    // 获取主循环状态
    bool isRunning() const;

private:
    // 将一行文本渲染到屏幕的指定位置
    // void renderText(const std::string& text, int x, int y, SDL_Color color);
    int renderTextWrapped(const std::string& text, int x, int y, int maxWidth, SDL_Color color);

    std::string windowTitle;
    int windowWidth;
    int windowHeight;

    bool running;

    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font; // 用于渲染字体的指针

    std::string currentInput; // 当前用户正在输入的文本
    struct ChatMessage {
        std::string author;
        std::string text;
    };
    std::vector<ChatMessage> chatHistory; // 存储聊天记录
};

#endif // WINDOW_MANAGER_HPP