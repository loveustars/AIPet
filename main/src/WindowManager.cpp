#include "WindowManager.hpp"
#include <iostream>
#include <sstream>

WindowManager::WindowManager(const std::string& title, int width, int height)
    : windowTitle(title), windowWidth(width), windowHeight(height), 
      running(false), window(nullptr), renderer(nullptr), font(nullptr) {
}

WindowManager::~WindowManager() {
    cleanup();
}

bool WindowManager::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 初始化 SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    font = TTF_OpenFont("/home/drew/Desktop/ai-desktop-c/assets/fonts/zhcn.ttf", 16); // 16 是字体大小
    if (!font) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }

    // 启用文本输入事件。这非常重要！
    SDL_StartTextInput();

    running = true;
    addMessage("System", "Welcome! Type your message and press Enter.");
    return true;
}

bool WindowManager::isRunning() const {
    return running;
}

std::optional<std::string> WindowManager::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        // 捕捉键盘按键事件
        else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_RETURN) { // 如果是回车键
                if (!currentInput.empty()) {
                    std::string submittedText = currentInput;
                    addMessage("You", submittedText);
                    currentInput.clear(); // 清空输入框
                    return submittedText; // 返回提交的文本
                }
            }
            else if (event.key.keysym.sym == SDLK_BACKSPACE && !currentInput.empty()) { // 如果是退格键
                // 移除最后一个字符 (UTF-8 字符可能占用多个字节，这里做简化处理)
                currentInput.pop_back();
            }
        }
        // 捕捉文本输入事件 (用于输入字符)
        else if (event.type == SDL_TEXTINPUT) {
            currentInput += event.text.text;
        }
    }
    return std::nullopt; // 没有提交文本
}

void WindowManager::update() {
    // 暂时为空
}

/*
void WindowManager::renderText(const std::string& text, int x, int y, SDL_Color color) {
    if (text.empty()) return;

    // 1. 使用 TTF 从文本创建 Surface
    SDL_Surface* textSurface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
    if (!textSurface) {
        std::cerr << "Unable to render text surface! TTF_Error: " << TTF_GetError() << std::endl;
        return;
    }

    // 2. 从 Surface 创建 Texture
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "Unable to create texture from rendered text! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }

    // 3. 设置渲染的目标矩形区域
    SDL_Rect renderQuad = {x, y, textSurface->w, textSurface->h};
    
    // 4. 渲染 Texture
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);

    // 5. 释放资源
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}*/

int WindowManager::renderTextWrapped(const std::string& text, int x, int y, int maxWidth, SDL_Color color) {
    if (text.empty() || !font) {
        return y;
    }

    std::stringstream ss(text);
    std::string word;
    std::string currentLine;
    int lineHeight = TTF_FontHeight(font) + 2; // 获取字体的高度并增加一点行距

    // 按单词拆分并构建每一行
    while (ss >> word) {
        // 如果是新的一行，直接加上单词
        if (currentLine.empty()) {
            currentLine = word;
        } else {
            // 否则，尝试加上 " " 和新单词
            std::string testLine = currentLine + " " + word;
            int textWidth = 0;
            // 使用 TTF_SizeUTF8 测量测试行的宽度
            if (TTF_SizeUTF8(font, testLine.c_str(), &textWidth, nullptr) != 0) {
                std::cerr << "TTF_SizeUTF8 Error: " << TTF_GetError() << std::endl;
                continue;
            }

            // 如果测试行没有超出最大宽度，就接受它
            if (textWidth <= maxWidth) {
                currentLine = testLine;
            } else {
                // 如果超出了，就先渲染已经构建好的当前行
                // (这里是渲染单行文本的逻辑)
                SDL_Surface* surface = TTF_RenderUTF8_Solid(font, currentLine.c_str(), color);
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_Rect dstRect = {x, y, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
                SDL_FreeSurface(surface);
                SDL_DestroyTexture(texture);

                // 移动到下一行，并用当前单词开始新的一行
                y += lineHeight;
                currentLine = word;
            }
        }
    }

    // 渲染最后剩下的一行
    if (!currentLine.empty()) {
        SDL_Surface* surface = TTF_RenderUTF8_Solid(font, currentLine.c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect dstRect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        y += lineHeight;
    }

    return y; // 返回下一行开始的 y 坐标
}

void WindowManager::render() {
    // 监听窗口尺寸变化
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // 设置背景色 (深灰色)
    SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
    SDL_RenderClear(renderer);

    // --- 开始渲染文本 ---
    int yOffset = 10;
    int margin = 10; // 左右边距
    int maxTextWidth = windowWidth - (margin * 2); // 文本区域的最大宽度

    SDL_Color authorColor = {100, 150, 255, 255}; // 蓝色
    SDL_Color textColor = {255, 255, 255, 255};   // 白色
    SDL_Color inputColor = {220, 220, 100, 255};  // 黄色

    // 渲染聊天记录
    for (const auto& msg : chatHistory) {
        std::string fullLine = msg.author + ": " + msg.text;
        SDL_Color currentColor = (msg.author == "You" || msg.author == "System") ? inputColor : textColor;
        
        // 调用新的换行函数
        yOffset = renderTextWrapped(fullLine, margin, yOffset, maxTextWidth, currentColor);
        yOffset += 5; // 在不同消息之间增加一点额外的间距
    }
    
    // 渲染分割线
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    // 确保分割线不会因为聊天内容过多而跑到屏幕外
    int separatorY = std::max(yOffset, windowHeight - 50);
    SDL_RenderDrawLine(renderer, 0, separatorY, windowWidth, separatorY);

    // 渲染当前输入行
    std::string inputLine = "> " + currentInput;
    // 同样使用换行函数来渲染输入行，以防输入过长
    renderTextWrapped(inputLine, margin, separatorY + 10, maxTextWidth, inputColor);

    SDL_RenderPresent(renderer);
}

/*
void WindowManager::render() {
    // 设置背景色 (深灰色)
    SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
    SDL_RenderClear(renderer);

    // --- 开始渲染文本 ---
    int yOffset = 10;
    SDL_Color authorColor = {100, 150, 255, 255}; // 蓝色
    SDL_Color textColor = {255, 255, 255, 255};   // 白色
    SDL_Color inputColor = {220, 220, 100, 255};  // 黄色

    // 渲染聊天记录
    for (const auto& msg : chatHistory) {
        std::string fullLine = msg.author + ": " + msg.text;
        renderText(fullLine, 10, yOffset, (msg.author == "You" ? inputColor : textColor));
        yOffset += 20; // 每行向下移动 20 像素
    }
    
    // 渲染分割线
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawLine(renderer, 0, yOffset + 5, windowWidth, yOffset + 5);
    yOffset += 20;

    // 渲染当前输入行
    std::string inputLine = "> " + currentInput;
    renderText(inputLine, 10, yOffset, inputColor);

    SDL_RenderPresent(renderer);
}
*/

void WindowManager::addMessage(const std::string& author, const std::string& text) {
    chatHistory.push_back({author, text});
    // 如果聊天记录太长，可以移除最旧的记录
    if (chatHistory.size() > 25) {
        chatHistory.erase(chatHistory.begin());
    }
}


void WindowManager::cleanup() {
    // 停止文本输入
    SDL_StopTextInput();

    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    
    TTF_Quit();
    SDL_Quit();
}