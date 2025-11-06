#include "WindowManager.hpp"
#include <iostream>
#include <GL/glew.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

WindowManager::WindowManager(const std::string& title, int width, int height, AIManager& manager)
    : windowTitle(title), windowWidth(width), windowHeight(height), 
      running(false), window(nullptr), glContext(nullptr), imgui_io(nullptr),
      aiManager(manager) {
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

    // ===== 关键修复：使用兼容性配置文件以支持 OpenGL ES 2.0 API =====
    std::cout << "[Debug] 2. Setting OpenGL attributes (Compatibility Profile for Live2D)..." << std::endl;
    const char* glsl_version = "#version 130";  // 对应 OpenGL 3.0
    
    // 使用兼容性配置文件而不是核心配置文件
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);  // 降低到 3.0
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    // 如果上面不行，可以尝试更低版本（取消注释下面的代码）
    /*
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    const char* glsl_version = "#version 120";
    */

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
    
    // ===== 清除 GLEW 初始化产生的错误（这是正常的）=====
    glGetError();
    
    std::cout << "[Debug] GLEW Initialized successfully." << std::endl;

    // 打印 OpenGL 信息
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << "[Debug] ---- OpenGL Info ----" << std::endl;
    std::cout << "[Debug]   Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "[Debug]   Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "[Debug]   Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "[Debug]   GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "[Debug]   Context Version: " << major << "." << minor << std::endl;
    std::cout << "[Debug] -----------------------" << std::endl;

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

    // 加载字体
    imgui_io->Fonts->AddFontFromFileTTF("assets/fonts/zhcn.ttf", 18.0f, nullptr, 
                                        imgui_io->Fonts->GetGlyphRangesChineseFull());
    std::cout << "[Debug] 8. Building ImGui font atlas..." << std::endl;
    if (!imgui_io->Fonts->Build()) {
        std::cerr << "Error: Failed to build font atlas!" << std::endl;
        return false;
    }
    std::cout << "[Debug] Font atlas built successfully." << std::endl;

    // 初始化 Live2D Manager
    std::cout << "[Debug] 9. Initializing Live2D Manager..." << std::endl;
    if (!_live2dManager.initialize(this)) {
        std::cerr << "Failed to initialize Live2DManager." << std::endl;
        return false;
    }

    // 加载模型
    std::cout << "[Debug] 10. Loading Live2D model..." << std::endl;
    if (!_live2dManager.loadModel("assets/live2d_models/hiyori_pro_t11")) {
        std::cerr << "Failed to load Live2D model." << std::endl;
    }

    std::cout << "[Debug] WindowManager initialized successfully!" << std::endl;
    running = true;
    addMessage("System", "Welcome! Type your message in English and press Enter.");
    return true;
}

bool WindowManager::isRunning() const {
    return running;
}

std::optional<std::string> WindowManager::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
            running = false;
        }
        if (event.type == SDL_WINDOWEVENT && 
            event.window.event == SDL_WINDOWEVENT_CLOSE && 
            event.window.windowID == SDL_GetWindowID(window)) {
            running = false;
        }
    }
    return std::nullopt;
}

void WindowManager::update() {
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    _live2dManager.update();
}

void WindowManager::render() {
    // 清空屏幕
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // ===== 检查 OpenGL 错误 =====
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[Render] OpenGL Error before Live2D draw: 0x" 
                  << std::hex << err << std::dec << std::endl;
    }
    
    // 绘制 Live2D 模型
    _live2dManager.draw();
    
    // ===== 检查 Live2D 渲染后的错误 =====
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[Render] OpenGL Error after Live2D draw: 0x" 
                  << std::hex << err << std::dec << std::endl;
    }
    
    // 开始 ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 绘制半透明聊天窗口
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));

        ImGui::Begin("Chat", nullptr, 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings);

        const float footerHeight = ImGui::GetStyle().ItemSpacing.y + 
                                   ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false, 
                         ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& msg : chatHistory) {
            ImVec4 color;
            if (msg.author == "You") {
                color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
            } else if (msg.author == "AI") {
                color = ImVec4(0.4f, 1.0f, 0.6f, 1.0f);
            } else {
                color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
            }
            
            ImGui::TextColored(color, "%s:", msg.author.c_str());
            ImGui::SameLine();
            ImGui::TextWrapped("%s", msg.text.c_str());
        }
        
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndChild();
        ImGui::Separator();

        if (ImGui::InputText("Input", currentInputBuffer, IM_ARRAYSIZE(currentInputBuffer), 
                            ImGuiInputTextFlags_EnterReturnsTrue)) {
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
        ImGui::PopStyleColor();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
    
    if (imgui_io) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        imgui_io = nullptr;
    }

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