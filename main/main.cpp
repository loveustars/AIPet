/**
 * @file main.cpp
 * @brief 集成版主程序：Live2D + AI 聊天
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <libgen.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Live2D
#include "LAppDefine.hpp"
#include "LAppAllocator_Common.hpp"
#include "LAppTextureManager.hpp"
#include "LAppPal.hpp"
#include "CubismUserModelExtend.hpp"
#include "MouseActionManager.hpp"

#include <CubismFramework.hpp>

// AI Manager
#include "AIManager.hpp"

// ===== 全局变量 =====
static GLFWwindow* g_Window = nullptr;
static int g_WindowWidth = 1280;
static int g_WindowHeight = 720;

static CubismUserModelExtend* g_UserModel = nullptr;
static LAppTextureManager* g_TextureManager = nullptr;
static LAppAllocator_Common g_CubismAllocator;
static Csm::CubismFramework::Option g_CubismOption;

static std::string g_ExecuteAbsolutePath;
static std::string g_CurrentModelDirectory;

// AI 相关
static AIManager* g_AIManager = nullptr;
static char g_InputBuffer[256] = {0};
static std::vector<std::pair<std::string, std::string>> g_ChatHistory;

// ImGui 字体
static ImFont* g_ChineseFont = nullptr;

// ===== 模型配置 =====
// 修改这里来切换不同的模型
static const char* MODEL_NAME = "Haru";  // 或 "hiyori_pro_t11"

/**
 * @brief 设置应用程序执行路径
 */
void SetExecuteAbsolutePath() {
    const int maximumPathBufferSize = 1024;
    char path[maximumPathBufferSize];
    ssize_t len = readlink("/proc/self/exe", path, maximumPathBufferSize - 1);
    
    if (len != -1) {
        path[len] = '\0';
    }
    
    g_ExecuteAbsolutePath = dirname(path);
    g_ExecuteAbsolutePath += "/";
}

/**
 * @brief 初始化 Cubism SDK
 */
void InitializeCubism() {
    g_CubismOption.LogFunction = LAppPal::PrintMessage;
    g_CubismOption.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;
    g_CubismOption.LoadFileFunction = LAppPal::LoadFileAsBytes;
    g_CubismOption.ReleaseBytesFunction = LAppPal::ReleaseBytes;
    
    Csm::CubismFramework::StartUp(&g_CubismAllocator, &g_CubismOption);
    Csm::CubismFramework::Initialize();
    
    std::cout << "[Cubism] Framework initialized" << std::endl;
}

/**
 * @brief 初始化系统
 */
bool InitializeSystem() {
    std::cout << "[System] Initializing..." << std::endl;
    
    // 初始化 GLFW
    std::cout << "[GLFW] Initializing..." << std::endl;
    if (glfwInit() == GL_FALSE) {
        std::cerr << "[Error] Failed to initialize GLFW" << std::endl;
        std::cerr << "[Debug] Make sure GLFW is properly installed" << std::endl;
        return false;
    }
    std::cout << "[GLFW] ✓ Initialized successfully" << std::endl;
    
    // 设置 OpenGL 版本 (3.3 Compatibility)
    std::cout << "[GLFW] Setting OpenGL hints..." << std::endl;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    
    // 创建窗口
    std::cout << "[GLFW] Creating window (" << g_WindowWidth << "x" << g_WindowHeight << ")..." << std::endl;
    g_Window = glfwCreateWindow(g_WindowWidth, g_WindowHeight, 
                                "AIPet - Live2D Desktop Pet", NULL, NULL);
    if (!g_Window) {
        std::cerr << "[Error] Failed to create GLFW window" << std::endl;
        std::cerr << "[Debug] Your system might not support OpenGL 3.3" << std::endl;
        
        // 尝试获取错误信息
        const char* description;
        int code = glfwGetError(&description);
        if (description) {
            std::cerr << "[GLFW Error " << code << "] " << description << std::endl;
        }
        
        glfwTerminate();
        return false;
    }
    std::cout << "[GLFW] ✓ Window created successfully" << std::endl;
    
    // 设置当前上下文
    std::cout << "[GLFW] Making context current..." << std::endl;
    glfwMakeContextCurrent(g_Window);
    glfwSwapInterval(1);  // 启用垂直同步
    std::cout << "[GLFW] ✓ Context set" << std::endl;
    
    // 初始化 GLEW
    std::cout << "[GLEW] Initializing..." << std::endl;
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "[Error] Failed to initialize GLEW" << std::endl;
        std::cerr << "[GLEW Error] " << glewGetErrorString(glewError) << std::endl;
        std::cerr << "[Debug] GLEW initialization failed with code: " << glewError << std::endl;
        
        // 打印 OpenGL 信息（如果可能）
        const GLubyte* version = glGetString(GL_VERSION);
        const GLubyte* vendor = glGetString(GL_VENDOR);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        
        if (version) std::cerr << "[Debug] GL Version: " << version << std::endl;
        if (vendor) std::cerr << "[Debug] GL Vendor: " << vendor << std::endl;
        if (renderer) std::cerr << "[Debug] GL Renderer: " << renderer << std::endl;
        
        glfwDestroyWindow(g_Window);
        glfwTerminate();
        return false;
    }
    std::cout << "[GLEW] ✓ Initialized successfully" << std::endl;
    
    // 清除 GLEW 初始化产生的错误
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "[Debug] Clearing GLEW initialization error: 0x" 
                  << std::hex << err << std::dec << std::endl;
    }
    
    // 打印 OpenGL 信息
    std::cout << "\n[OpenGL Info]" << std::endl;
    std::cout << "  Vendor:   " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "  Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "  Version:  " << glGetString(GL_VERSION) << std::endl;
    std::cout << "  GLSL:     " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << "  Context:  " << major << "." << minor << std::endl;
    std::cout << std::endl;
    
    // OpenGL 设置
    std::cout << "[OpenGL] Setting up rendering parameters..." << std::endl;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    std::cout << "[OpenGL] ✓ Rendering parameters set" << std::endl;
    
    // 设置视口
    std::cout << "[OpenGL] Setting viewport..." << std::endl;
    glfwGetWindowSize(g_Window, &g_WindowWidth, &g_WindowHeight);
    glViewport(0, 0, g_WindowWidth, g_WindowHeight);
    std::cout << "[OpenGL] ✓ Viewport set to " << g_WindowWidth << "x" << g_WindowHeight << std::endl;
    
    // 初始化 ImGui
    std::cout << "[ImGui] Initializing..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    std::cout << "[ImGui] ✓ Context created" << std::endl;
    
    // 初始化 ImGui 后端
    std::cout << "[ImGui] Initializing backends..." << std::endl;
    if (!ImGui_ImplGlfw_InitForOpenGL(g_Window, true)) {
        std::cerr << "[Error] Failed to initialize ImGui GLFW backend" << std::endl;
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "[Error] Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }
    std::cout << "[ImGui] ✓ Backends initialized" << std::endl;
    
    // 加载中文字体
    std::cout << "[ImGui] Loading Chinese font..." << std::endl;
    std::string fontPath = g_ExecuteAbsolutePath + "assets/fonts/zhcn.ttf";
    std::cout << "[Debug] Font path: " << fontPath << std::endl;
    
    g_ChineseFont = io.Fonts->AddFontFromFileTTF(
        fontPath.c_str(), 18.0f, nullptr, 
        io.Fonts->GetGlyphRangesChineseFull()
    );
    
    if (!g_ChineseFont) {
        std::cerr << "[Warning] Failed to load Chinese font from: " << fontPath << std::endl;
        std::cerr << "[Info] Will use default font (no Chinese support)" << std::endl;
    } else {
        std::cout << "[ImGui] ✓ Chinese font loaded" << std::endl;
    }
    
    // 初始化 Cubism
    std::cout << "[Cubism] Initializing SDK..." << std::endl;
    InitializeCubism();
    std::cout << "[Cubism] ✓ SDK initialized" << std::endl;
    
    // 初始化鼠标管理器
    std::cout << "[MouseManager] Initializing..." << std::endl;
    MouseActionManager::GetInstance()->Initialize(g_WindowWidth, g_WindowHeight);
    std::cout << "[MouseManager] ✓ Initialized" << std::endl;
    
    // 注册回调
    std::cout << "[GLFW] Registering callbacks..." << std::endl;
    glfwSetMouseButtonCallback(g_Window, EventHandler::OnMouseCallBack);
    glfwSetCursorPosCallback(g_Window, EventHandler::OnMouseCallBack);
    std::cout << "[GLFW] ✓ Callbacks registered" << std::endl;
    
    std::cout << "[Debug] Getting executable path..." << std::endl;
    SetExecuteAbsolutePath();
    std::cout << "[Debug] Executable path: " << g_ExecuteAbsolutePath << std::endl;
    
    std::cout << "[System] Initialization complete" << std::endl;
    return true;
}

/**
 * @brief 加载 Live2D 模型
 */
bool LoadModel(const std::string& modelName) {
    std::cout << "\n[Model] Loading: " << modelName << std::endl;
    
    // 设置模型目录
    g_CurrentModelDirectory = g_ExecuteAbsolutePath + "assets/live2d_models/" + modelName + "/";
    std::cout << "[Debug] Model directory: " << g_CurrentModelDirectory << std::endl;
    
    // 检查目录是否存在
    std::string modelJsonFile = g_CurrentModelDirectory + modelName + ".model3.json";
    std::cout << "[Debug] Model JSON file: " << modelJsonFile << std::endl;
    
    std::ifstream checkFile(modelJsonFile);
    if (!checkFile.good()) {
        std::cerr << "[Error] Model file not found: " << modelJsonFile << std::endl;
        std::cerr << "[Info] Please ensure the model files are in: " << g_CurrentModelDirectory << std::endl;
        return false;
    }
    checkFile.close();
    std::cout << "[Model] ✓ Model file exists" << std::endl;
    
    // 创建用户模型
    std::cout << "[Model] Creating user model..." << std::endl;
    try {
        g_UserModel = new CubismUserModelExtend(modelName, g_CurrentModelDirectory);
    } catch (const std::exception& e) {
        std::cerr << "[Error] Failed to create user model: " << e.what() << std::endl;
        return false;
    }
    std::cout << "[Model] ✓ User model created" << std::endl;
    
    // 加载模型资源
    std::cout << "[Model] Loading assets..." << std::endl;
    std::string jsonFileName = modelName + ".model3.json";
    try {
        g_UserModel->LoadAssets(jsonFileName.c_str());
    } catch (const std::exception& e) {
        std::cerr << "[Error] Failed to load model assets: " << e.what() << std::endl;
        return false;
    }
    std::cout << "[Model] ✓ Assets loaded" << std::endl;
    
    // 设置用户模型到鼠标管理器
    std::cout << "[Model] Setting model to MouseActionManager..." << std::endl;
    MouseActionManager::GetInstance()->SetUserModel(g_UserModel);
    std::cout << "[Model] ✓ Model set successfully" << std::endl;
    
    std::cout << "[Model] ✓✓✓ Model loaded successfully ✓✓✓\n" << std::endl;
    return true;
}

/**
 * @brief 初始化 AI 管理器
 */
bool InitializeAI() {
    std::cout << "[AI] Initializing..." << std::endl;
    
    const char* apiKey = std::getenv("GOOGLE_AI_STUDIO_API_KEY");
    if (!apiKey || strlen(apiKey) == 0) {
        std::cerr << "[Warning] GOOGLE_AI_STUDIO_API_KEY not set" << std::endl;
        std::cerr << "[Info] AI chat features will not work without API key" << std::endl;
        std::cerr << "[Info] Set it with: export GOOGLE_AI_STUDIO_API_KEY='your-key'" << std::endl;
        apiKey = "";
    } else {
        std::cout << "[AI] ✓ API key found (length: " << strlen(apiKey) << ")" << std::endl;
    }
    
    g_AIManager = new AIManager(apiKey);
    g_ChatHistory.push_back({"System", "Welcome! AI Desktop Pet is ready."});
    g_ChatHistory.push_back({"System", "Type a message and press Enter to chat."});
    
    std::cout << "[AI] ✓ Initialized" << std::endl;
    return true;
}

/**
 * @brief 渲染 ImGui 聊天界面
 */
void RenderChatUI() {
    if (g_ChineseFont) {
        ImGui::PushFont(g_ChineseFont);
    }
    
    // 创建半透明聊天窗口
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(400, 500));
    ImGui::SetNextWindowBgAlpha(0.85f);
    
    ImGui::Begin("AI Chat", nullptr, 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    
    // 聊天记录显示区域
    ImGui::BeginChild("ChatHistory", ImVec2(0, -30), true);
    
    for (const auto& msg : g_ChatHistory) {
        ImVec4 color = (msg.first == "You") ? 
            ImVec4(0.4f, 0.8f, 1.0f, 1.0f) : 
            (msg.first == "AI") ?
            ImVec4(0.4f, 1.0f, 0.6f, 1.0f) :
            ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
        
        ImGui::TextColored(color, "%s:", msg.first.c_str());
        ImGui::TextWrapped("%s", msg.second.c_str());
        ImGui::Separator();
    }
    
    // 自动滚动到底部
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    // 输入框
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##input", g_InputBuffer, sizeof(g_InputBuffer), 
                        ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (strlen(g_InputBuffer) > 0) {
            std::string userInput = g_InputBuffer;
            g_ChatHistory.push_back({"You", userInput});
            
            if (!g_AIManager->isBusy()) {
                g_AIManager->sendMessage(userInput);
            } else {
                g_ChatHistory.push_back({"System", "AI is busy, please wait..."});
            }
            
            memset(g_InputBuffer, 0, sizeof(g_InputBuffer));
        }
    }
    
    ImGui::End();
    
    if (g_ChineseFont) {
        ImGui::PopFont();
    }
}

/**
 * @brief 更新逻辑
 */
void Update() {
    // 更新时间
    LAppPal::UpdateTime();
    
    // 检查窗口大小变化
    int width, height;
    glfwGetWindowSize(g_Window, &width, &height);
    if (width != g_WindowWidth || height != g_WindowHeight) {
        g_WindowWidth = width;
        g_WindowHeight = height;
        glViewport(0, 0, width, height);
        MouseActionManager::GetInstance()->ViewInitialize(width, height);
    }
    
    // 检查 AI 响应
    if (g_AIManager) {
        std::string response = g_AIManager->getResult();
        if (!response.empty()) {
            g_ChatHistory.push_back({"AI", response});
        }
    }
}

/**
 * @brief 渲染
 */
void Render() {
    // 清空屏幕
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearDepth(1.0);
    
    // 更新并绘制 Live2D 模型
    if (g_UserModel) {
        g_UserModel->ModelOnUpdate(g_Window);
    }
    
    // 开始 ImGui 帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // 渲染聊天界面
    RenderChatUI();
    
    // 渲染 ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // 交换缓冲区
    glfwSwapBuffers(g_Window);
}

/**
 * @brief 主循环
 */
void Run() {
    std::cout << "[App] Starting main loop" << std::endl;
    
    while (!glfwWindowShouldClose(g_Window)) {
        // 更新
        Update();
        
        // 渲染
        Render();
        
        // 处理事件
        glfwPollEvents();
    }
}

/**
 * @brief 清理资源
 */
void Cleanup() {
    std::cout << "[App] Cleaning up..." << std::endl;
    
    // 清理 AI
    if (g_AIManager) {
        delete g_AIManager;
        g_AIManager = nullptr;
    }
    
    // 清理 Live2D 模型
    if (g_UserModel) {
        g_UserModel->DeleteRenderer();
        delete g_UserModel;
        g_UserModel = nullptr;
    }
    
    // 清理纹理管理器
    if (g_TextureManager) {
        delete g_TextureManager;
        g_TextureManager = nullptr;
    }
    
    // 清理鼠标管理器
    MouseActionManager::ReleaseInstance();
    
    // 清理 ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // 清理 Cubism
    Csm::CubismFramework::Dispose();
    
    // 清理 GLFW
    if (g_Window) {
        glfwDestroyWindow(g_Window);
        g_Window = nullptr;
    }
    glfwTerminate();
    
    std::cout << "[App] Cleanup complete" << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   AIPet - Live2D Desktop Pet with AI  " << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 初始化系统
    if (!InitializeSystem()) {
        std::cerr << "[Error] Failed to initialize system" << std::endl;
        return -1;
    }
    
    // 加载模型
    if (!LoadModel(MODEL_NAME)) {
        std::cerr << "[Error] Failed to load model" << std::endl;
        Cleanup();
        return -1;
    }
    
    // 初始化 AI
    if (!InitializeAI()) {
        std::cerr << "[Error] Failed to initialize AI" << std::endl;
        Cleanup();
        return -1;
    }
    
    // 运行主循环
    Run();
    
    // 清理资源
    Cleanup();
    
    return 0;
}