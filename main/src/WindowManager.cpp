
#include "WindowManager.hpp"
#include <iostream>
#include <GL/glew.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

WindowManager::WindowManager(const std::string& title, int width, int height, AIManager& manager)
    : windowTitle(title), windowWidth(width), windowHeight(height), 
      running(false), window(nullptr), glContext(nullptr), imgui_io(nullptr),
      aiManager(manager) { // 初始化引用
}

WindowManager::~WindowManager() {
    cleanup();
}

bool WindowManager::init() {
    std::cout << "[Debug] 1. Initializing SDL..." << std::endl;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Error: SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "[Debug] SDL Initialized successfully." << std::endl;


    std::cout << "[Debug] 2. Setting OpenGL attributes (Requesting Core 3.3)..." << std::endl;
    const char* glsl_version = "#version 330";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);


    std::cout << "[Debug] 3. Creating SDL Window..." << std::endl;
    window = SDL_CreateWindow(
        windowTitle.c_str(), 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        windowWidth, windowHeight, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        std::cerr << "Error: Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "[Debug] SDL Window created successfully." << std::endl;


    std::cout << "[Debug] 4. Creating OpenGL Context..." << std::endl;
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Error: OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        std::cerr << "Hint: Your system/VM might not support OpenGL 3.3. Check your graphics drivers." << std::endl;
        return false;
    }
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);
    std::cout << "[Debug] OpenGL Context created successfully." << std::endl;


    std::cout << "[Debug] 5. Initializing GLEW..." << std::endl;
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Error: Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return false;
    }
    std::cout << "[Debug] GLEW Initialized successfully." << std::endl;


    // --- 额外添加：打印实际获取到的 OpenGL 版本信息 ---
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << "[Debug] ---- OpenGL Info ----" << std::endl;
    std::cout << "[Debug]   Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "[Debug]   Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "[Debug]   Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "[Debug]   Context Version (queried): " << major << "." << minor << std::endl;
    std::cout << "[Debug] -----------------------" << std::endl;
    if (major < 3 || (major == 3 && minor < 3)) {
        std::cerr << "[Warning] The created OpenGL context version is less than 3.3. This might cause issues." << std::endl;
    }


    std::cout << "[Debug] 6. Initializing Dear ImGui..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imgui_io = &ImGui::GetIO();
    ImGui::StyleColorsDark();


    std::cout << "[Debug] 7. Initializing ImGui Backends..." << std::endl;
    if (!ImGui_ImplSDL2_InitForOpenGL(window, glContext)) {
        std::cerr << "Error: Failed to initialize ImGui SDL2 backend!" << std::endl;
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::cerr << "Error: Failed to initialize ImGui OpenGL3 backend!" << std::endl;
        return false;
    }
    std::cout << "[Debug] ImGui Backends initialized successfully." << std::endl;


    // 在这里更改字体，后续可以加入字体选择接口
    imgui_io->Fonts->AddFontFromFileTTF("assets/fonts/zhcn.ttf", 18.0f, nullptr, imgui_io->Fonts->GetGlyphRangesChineseFull());
    std::cout << "[Debug] 8. Building ImGui font atlas (using default font)..." << std::endl;
    if (!imgui_io->Fonts->Build()) {
        std::cerr << "Error: Failed to build font atlas even with the default font!" << std::endl;
        return false;
    }
    std::cout << "[Debug] Font atlas built successfully." << std::endl;

    // 初始化Live2D Manager
    std::cout << "[Debug] 9. Initializing Live2D Manager..." << std::endl;
    if (!_live2dManager.initialize(this)) {
        std::cerr << "Failed to initialize Live2DManager." << std::endl;
        return false;
    }

    // --- 加载你的第一个模型！---
    // !!! 请确保你已经把模型文件放到了这个路径下 !!!
    std::cout << "[Debug] 10. Loading Live2D model..." << std::endl;
    if (!_live2dManager.loadModel("assets/live2d_models/Haru")) {
        std::cerr << "Failed to load Live2D model." << std::endl;
        // 这里我们不返回 false，即使模型加载失败，程序也可以继续运行
    }

    std::cout << "[Debug] WindowManager initialized successfully!" << std::endl;
    running = true;
    addMessage("System", "Welcome! Type your message in English and press Enter.");
    return true;
}

/**
 * @file WindowManager.cpp
 */
/*
#include "WindowManager.hpp"
#include <iostream>
#include <string>
#include <GL/glew.h>      // GLEW
#include "imgui.h"        // Dear ImGui
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

WindowManager::WindowManager(const std::string& title, int width, int height)
    : windowTitle(title), windowWidth(width), windowHeight(height), 
      running(false), window(nullptr), glContext(nullptr), imgui_io(nullptr) {
}

WindowManager::~WindowManager() {
    cleanup();
}

/*
bool WindowManager::init() {
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 设置 OpenGL 版本和属性 (核心模式 3.3)
    const char* glsl_version = "#version 330";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // 设置双缓冲
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // 创建带 OpenGL 上下文的窗口
    window = SDL_CreateWindow(
        windowTitle.c_str(), 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        windowWidth, windowHeight, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 创建 OpenGL 上下文并设为当前
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1); // 开启垂直同步

    // 初始化 GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return false;
    }

    // 初始化 Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imgui_io = &ImGui::GetIO(); (void)imgui_io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    
    ImGui::StyleColorsDark(); // 设置 ImGui 风格

    // 绑定 ImGui 到 SDL2 和 OpenGL3
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // 加载字体
    // 注意：请确保你的字体路径正确
    imgui_io->Fonts->AddFontFromFileTTF("assets/fonts/zhcn.ttf", 18.0f, nullptr, imgui_io->Fonts->GetGlyphRangesChineseFull());
    
    if (!imgui_io->Fonts->IsBuilt()) {
        std::cerr << "Failed to build font atlas!" << std::endl;
        return false;
    }

    running = true;
    addMessage("System", "Welcome! Type your message and press Enter.");
    return true;
}*/

bool WindowManager::isRunning() const {
    return running;
}

std::optional<std::string> WindowManager::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // ImGui 处理事件
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
            running = false;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
            running = false;
        }
    }
    // 注意：ImGui 的输入框会自己处理回车事件，所以我们把逻辑移到 render 函数里
    return std::nullopt;
}

void WindowManager::update() {
    // 监听窗口尺寸变化，更新视口
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    _live2dManager.update(); // 每帧更新 Live2D
}



/*
void WindowManager::render() {
    // 开始新的一帧 ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 在这里绘制Live2D 模型
    // TODO: Render Live2D Model here.(Not Done)
    // 在绘制 UI 之前绘制模型，UI 就会自然地显示在模型之上。
    _live2dManager.draw();

    // 使用 ImGui 构建我们的聊天窗口
    {
        // 设置窗口位置和大小，使其填充整个应用窗口
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));

        // 创建一个无边框、不可移动的背景窗口
        ImGui::Begin("Chat", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        // 创建一个子区域来显示聊天记录，并留出底部空间给输入框
        const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

        // 渲染聊天记录
        for (const auto& msg : chatHistory) {
            ImVec4 color = (msg.author == "You") ? ImVec4(0.9f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::TextColored(color, "%s:", msg.author.c_str());
            ImGui::SameLine();
            ImGui::TextWrapped("%s", msg.text.c_str());
        }
        
        // 自动滚动到底部
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndChild();
        ImGui::Separator();

        
        if (ImGui::InputText("Input", currentInputBuffer, IM_ARRAYSIZE(currentInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (strlen(currentInputBuffer) > 0) {
                std::string submittedText = currentInputBuffer;
                addMessage("You", submittedText);
                if (!aiManager.isBusy()) {
                    aiManager.sendMessage(submittedText);
                } else {
                    addMessage("System", "AI is busy, please wait...");
                }
                memset(currentInputBuffer, 0, sizeof(currentInputBuffer));
            }
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::End();
    }
    
    // 清空屏幕并渲染
    // 设置清空颜色为透明，这样桌宠背景才能透明
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 交换缓冲区
    SDL_GL_SwapWindow(window);
}*/

void WindowManager::render() {
    // 清空屏幕为深色背景，便于看到 Live2D 模型
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 绘制 Live2D 模型（在 UI 之前绘制，这样 UI 会显示在模型上方）
    _live2dManager.draw();
    
    // 开始新的一帧 ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 绘制半透明的聊天窗口
    {
        // 设置窗口位置和大小
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
        
        // 设置窗口背景为半透明
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // 30% 透明度

        // 创建窗口
        ImGui::Begin("Chat", nullptr, 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings);

        // 创建聊天记录显示区域
        const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

        // 渲染聊天记录
        for (const auto& msg : chatHistory) {
            ImVec4 color;
            if (msg.author == "You") {
                color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);  // 浅蓝色
            } else if (msg.author == "AI") {
                color = ImVec4(0.4f, 1.0f, 0.6f, 1.0f);  // 浅绿色
            } else {
                color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);  // 黄色（系统消息）
            }
            
            ImGui::TextColored(color, "%s:", msg.author.c_str());
            ImGui::SameLine();
            ImGui::TextWrapped("%s", msg.text.c_str());
        }
        
        // 自动滚动到底部
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndChild();
        ImGui::Separator();

        // 输入框
        if (ImGui::InputText("Input", currentInputBuffer, IM_ARRAYSIZE(currentInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (strlen(currentInputBuffer) > 0) {
                std::string submittedText = currentInputBuffer;
                addMessage("You", submittedText);
                
                if (!aiManager.isBusy()) {
                    aiManager.sendMessage(submittedText);
                } else {
                    addMessage("System", "AI is busy, please wait...");
                }
                
                memset(currentInputBuffer, 0, sizeof(currentInputBuffer));
            }
            ImGui::SetKeyboardFocusHere(-1);
        }
        
        ImGui::End();
        ImGui::PopStyleColor(); // 恢复窗口背景色
    }
    
    // 渲染 ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 交换缓冲区
    SDL_GL_SwapWindow(window);
}

void WindowManager::getWindowSize(int& width, int& height) const {
    if (window) {
        SDL_GetWindowSize(window, &width, &height);
    } else {
        width = 0;
        height = 0;
    }
}

void WindowManager::addMessage(const std::string& author, const std::string& text) {
    chatHistory.push_back({author, text});
    if (chatHistory.size() > 50) {
        chatHistory.erase(chatHistory.begin());
    }
}

void WindowManager::cleanup() {
    _live2dManager.cleanup();
    // 清理 ImGui
    if (imgui_io) { // 检查 imgui_io 是否已初始化
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        imgui_io = nullptr;
    }

    // 清理 SDL 和 OpenGL
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    
    SDL_Quit();
}

/*
void WindowManager::cleanup() {
    // 清理 ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // 清理 SDL 和 OpenGL
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    
    SDL_Quit();
}*/