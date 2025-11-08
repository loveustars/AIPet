/**
 * @file main.cpp
 * @author Drew Nick
 * @brief 双窗口版本：Live2D窗口 + 独立聊天窗口
 * @note 修复了鼠标交互导致的卡死问题(部分修复)
 * @note 窗口支持透明背景和窗口装饰
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <libgen.h>
#include <thread>
#include <atomic>
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

// 全局变量
// Live2D 窗口
static GLFWwindow* g_MainWindow = nullptr;
static int g_MainWindowWidth = 800;
static int g_MainWindowHeight = 600;

// 聊天窗口
static GLFWwindow* g_ChatWindow = nullptr;
static int g_ChatWindowWidth = 500;
static int g_ChatWindowHeight = 600;

static CubismUserModelExtend* g_UserModel = nullptr;
static LAppTextureManager* g_TextureManager = nullptr;
static LAppAllocator_Common g_CubismAllocator;
static Csm::CubismFramework::Option g_CubismOption;

static std::string g_ExecuteAbsolutePath;
static std::string g_CurrentModelDirectory;

// AI 相关
static AIManager* g_AIManager = nullptr;
static char g_InputBuffer[512] = {0};
static std::vector<std::pair<std::string, std::string>> g_ChatHistory;

// ImGui 字体
static ImFont* g_ChineseFont = nullptr;

// 窗口关闭标志
static std::atomic<bool> g_ShouldClose(false);

// 模型配置
static const char* MODEL_NAME = "hiyori_pro_t11";

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
 * @brief 验证OpenGL上下文是否可用
 */
bool VerifyOpenGLContext() {
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    
    if (!version || !vendor || !renderer) {
        return false;
    }
    
    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    GLenum err = glGetError();
    if (err != GL_NO_ERROR && err != GL_INVALID_ENUM) {
        return false;
    }
    
    return true;
}

/**
 * @brief 主窗口鼠标按钮回调 - 修复版
 */
void MainWindowMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // 确保当前上下文正确
    glfwMakeContextCurrent(window);
    
    // 调用Live2D的鼠标处理
    if (g_UserModel) {
        EventHandler::OnMouseCallBack(window, button, action, mods);
    }
}

/**
 * @brief 主窗口鼠标移动回调
 */
void MainWindowCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // 确保当前上下文正确
    glfwMakeContextCurrent(window);
    
    // 调用Live2D的鼠标处理
    if (g_UserModel) {
        EventHandler::OnMouseCallBack(window, xpos, ypos);
    }
}

/**
 * @brief 初始化主窗口（Live2D）
 */
bool InitializeMainWindow() {
    std::cout << "[MainWindow] Initializing..." << std::endl;
    
    if (glfwInit() == GL_FALSE) {
        std::cerr << "[Error] Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // OpenGL 设置
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    
    // 窗口设置 - 支持透明背景
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);  // 显示窗口装饰（标题栏、边框）
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);  // 允许调整大小
    glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);  // 不置顶
    
    g_MainWindow = glfwCreateWindow(g_MainWindowWidth, g_MainWindowHeight, 
                                    "AIPet - Live2D Model", NULL, NULL);
    if (!g_MainWindow) {
        std::cerr << "[Error] Failed to create main window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(g_MainWindow);
    glfwSwapInterval(1);
    
    // 初始化 GLEW（容错版本）
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cout << "[Warning] GLEW initialization error: " << glewError << std::endl;
        if (!VerifyOpenGLContext()) {
            std::cerr << "[Error] OpenGL context is not functional" << std::endl;
            return false;
        }
        std::cout << "[GLEW] ✓ Continuing with functional OpenGL context" << std::endl;
    }
    
    // 清除错误
    while (glGetError() != GL_NO_ERROR);
    
    // OpenGL 设置
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, g_MainWindowWidth, g_MainWindowHeight);
    
    // 键盘回调
    glfwSetKeyCallback(g_MainWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            g_ShouldClose = true;
        }
    });
    
    std::cout << "[MainWindow] ✓ Initialized successfully" << std::endl;
    std::cout << "[MainWindow] Transparent background: ENABLED" << std::endl;
    std::cout << "[MainWindow] Window decorations: ENABLED" << std::endl;
    return true;
}

/**
 * @brief 初始化聊天窗口
 */
bool InitializeChatWindow() {
    std::cout << "[ChatWindow] Initializing..." << std::endl;
    
    // 创建共享上下文的窗口
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // 聊天窗口位置在屏幕右侧
    g_ChatWindow = glfwCreateWindow(g_ChatWindowWidth, g_ChatWindowHeight, 
                                    "AIPet - AI Chat", NULL, g_MainWindow);
    if (!g_ChatWindow) {
        std::cerr << "[Error] Failed to create chat window" << std::endl;
        return false;
    }
    
    // 设置聊天窗口位置（主窗口右侧）
    int xpos, ypos;
    glfwGetWindowPos(g_MainWindow, &xpos, &ypos);
    glfwSetWindowPos(g_ChatWindow, xpos + g_MainWindowWidth + 20, ypos);
    
    glfwMakeContextCurrent(g_ChatWindow);
    glfwSwapInterval(1);
    
    // 初始化 ImGui（聊天窗口专用）
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    
    if (!ImGui_ImplGlfw_InitForOpenGL(g_ChatWindow, true)) {
        std::cerr << "[Error] Failed to initialize ImGui for chat window" << std::endl;
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "[Error] Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }
    
    // 加载中文字体
    std::string fontPath = g_ExecuteAbsolutePath + "assets/fonts/zhcn.ttf";
    g_ChineseFont = io.Fonts->AddFontFromFileTTF(
        fontPath.c_str(), 18.0f, nullptr, 
        io.Fonts->GetGlyphRangesChineseFull()
    );
    
    if (!g_ChineseFont) {
        std::cerr << "[Warning] Failed to load Chinese font" << std::endl;
    }
    
    // 键盘回调
    glfwSetKeyCallback(g_ChatWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            g_ShouldClose = true;
        }
    });
    
    std::cout << "[ChatWindow] ✓ Initialized successfully" << std::endl;
    return true;
}

/**
 * @brief 初始化Live2D
 */
bool InitializeLive2D() {
    std::cout << "[Live2D] Initializing..." << std::endl;
    
    SetExecuteAbsolutePath();
    InitializeCubism();
    
    // 切换到主窗口上下文
    glfwMakeContextCurrent(g_MainWindow);
    
    // 初始化鼠标管理器
    MouseActionManager::GetInstance()->Initialize(g_MainWindowWidth, g_MainWindowHeight);
    
    // 注册回调
    glfwSetMouseButtonCallback(g_MainWindow, MainWindowMouseButtonCallback);
    glfwSetCursorPosCallback(g_MainWindow, MainWindowCursorPosCallback);
    
    // 加载模型
    g_CurrentModelDirectory = g_ExecuteAbsolutePath + "assets/live2d_models/" + std::string(MODEL_NAME) + "/";
    
    // 检查模型文件
    std::string modelJsonPath = g_CurrentModelDirectory + std::string(MODEL_NAME) + ".model3.json";
    std::ifstream testFile(modelJsonPath);
    if (!testFile.good()) {
        std::cerr << "[Error] Model file not found: " << modelJsonPath << std::endl;
        return false;
    }
    testFile.close();
    
    g_TextureManager = new LAppTextureManager();
    g_UserModel = new CubismUserModelExtend(MODEL_NAME, g_CurrentModelDirectory);
    
    std::string jsonFileName = std::string(MODEL_NAME) + ".model3.json";
    try {
        g_UserModel->LoadAssets(jsonFileName.c_str());
    } catch (const std::exception& e) {
        std::cerr << "[Error] Failed to load model: " << e.what() << std::endl;
        return false;
    }
    
    MouseActionManager::GetInstance()->SetUserModel(g_UserModel);
    
    std::cout << "[Live2D] ✓ Initialized successfully" << std::endl;
    return true;
}

/**
 * @brief 初始化 AI
 */
bool InitializeAI() {
    std::cout << "[AI] Initializing..." << std::endl;
    
    const char* apiKey = std::getenv("GOOGLE_AI_STUDIO_API_KEY");
    if (!apiKey || strlen(apiKey) == 0) {
        std::cerr << "[Warning] GOOGLE_AI_STUDIO_API_KEY not set" << std::endl;
        apiKey = "";
    }
    
    g_AIManager = new AIManager(apiKey);
    g_ChatHistory.push_back({"System", "Welcome! Dual-window AI Desktop Pet."});
    g_ChatHistory.push_back({"System", "Live2D window: Transparent background with decorations."});
    g_ChatHistory.push_back({"System", "Press ESC in any window to exit."});
    
    std::cout << "[AI] ✓ Initialized" << std::endl;
    return true;
}

/**
 * @brief 渲染主窗口（Live2D）
 */
void RenderMainWindow() {
    // 确保切换到主窗口上下文
    glfwMakeContextCurrent(g_MainWindow);
    
    // 检查窗口大小变化
    int width, height;
    glfwGetWindowSize(g_MainWindow, &width, &height);
    if (width != g_MainWindowWidth || height != g_MainWindowHeight) {
        g_MainWindowWidth = width;
        g_MainWindowHeight = height;
        glViewport(0, 0, width, height);
        if (MouseActionManager::GetInstance()) {
            MouseActionManager::GetInstance()->ViewInitialize(width, height);
        }
    }
    
    // 清屏 - 使用半透明灰色背景
    glClearColor(0.15f, 0.15f, 0.15f, 0.7f);  // 半透明深灰色
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearDepth(1.0);
    
    // 更新并渲染Live2D模型
    if (g_UserModel) {
        try {
            g_UserModel->ModelOnUpdate(g_MainWindow);
        } catch (const std::exception& e) {
            std::cerr << "[Error] Model update failed: " << e.what() << std::endl;
        }
    }
    
    glfwSwapBuffers(g_MainWindow);
}

/**
 * @brief 渲染聊天窗口
 */
void RenderChatWindow() {
    glfwMakeContextCurrent(g_ChatWindow);
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    if (g_ChineseFont) {
        ImGui::PushFont(g_ChineseFont);
    }
    
    // 全屏ImGui窗口
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Chat", nullptr, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse);
    
    // 标题和控制
    ImGui::Text("AI Chat Window");
    ImGui::SameLine(ImGui::GetWindowWidth() - 150);
    if (ImGui::Button("Clear", ImVec2(70, 0))) {
        g_ChatHistory.clear();
        g_ChatHistory.push_back({"System", "Chat cleared."});
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit", ImVec2(70, 0))) {
        g_ShouldClose = true;
    }
    
    ImGui::Separator();
    
    // 聊天历史
    ImGui::BeginChild("History", ImVec2(0, -40), true);
    
    for (const auto& msg : g_ChatHistory) {
        ImVec4 color = (msg.first == "You") ? 
            ImVec4(0.4f, 0.8f, 1.0f, 1.0f) : 
            (msg.first == "AI") ?
            ImVec4(0.4f, 1.0f, 0.6f, 1.0f) :
            ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
        
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextWrapped("[%s] %s", msg.first.c_str(), msg.second.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }
    
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    // 输入区
    ImGui::Separator();
    ImGui::SetNextItemWidth(-80);
    bool send = ImGui::InputText("##input", g_InputBuffer, sizeof(g_InputBuffer), 
                    ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    send |= ImGui::Button("Send", ImVec2(70, 0));
    
    if (send && strlen(g_InputBuffer) > 0) {
        std::string input = g_InputBuffer;
        g_ChatHistory.push_back({"You", input});
        
        if (g_AIManager && !g_AIManager->isBusy()) {
            g_AIManager->sendMessage(input);
        } else {
            g_ChatHistory.push_back({"System", "AI is busy..."});
        }
        
        memset(g_InputBuffer, 0, sizeof(g_InputBuffer));
    }
    
    ImGui::End();
    
    if (g_ChineseFont) {
        ImGui::PopFont();
    }
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glfwSwapBuffers(g_ChatWindow);
}

/**
 * @brief 主循环
 */
void Run() {
    std::cout << "[App] Starting dual-window loop" << std::endl;
    std::cout << "[Info] Live2D window: Semi-transparent gray background" << std::endl;
    std::cout << "[Info] Both windows have decorations and can be dragged" << std::endl;
    std::cout << "[Info] Press ESC in any window to exit" << std::endl;
    
    while (!g_ShouldClose && !glfwWindowShouldClose(g_MainWindow) && !glfwWindowShouldClose(g_ChatWindow)) {
        // 更新时间
        LAppPal::UpdateTime();
        
        // 检查 AI 响应
        if (g_AIManager) {
            std::string response = g_AIManager->getResult();
            if (!response.empty()) {
                g_ChatHistory.push_back({"AI", response});
            }
        }
        
        // 渲染两个窗口
        RenderMainWindow();
        RenderChatWindow();
        
        // 处理事件
        glfwPollEvents();
    }
}

/**
 * @brief 清理资源
 */
void Cleanup() {
    std::cout << "[App] Cleaning up..." << std::endl;
    
    if (g_AIManager) {
        delete g_AIManager;
        g_AIManager = nullptr;
    }
    
    if (g_UserModel) {
        glfwMakeContextCurrent(g_MainWindow);
        g_UserModel->DeleteRenderer();
        delete g_UserModel;
        g_UserModel = nullptr;
    }
    
    if (g_TextureManager) {
        delete g_TextureManager;
        g_TextureManager = nullptr;
    }
    
    MouseActionManager::ReleaseInstance();
    
    // 清理聊天窗口
    if (g_ChatWindow) {
        glfwMakeContextCurrent(g_ChatWindow);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(g_ChatWindow);
        g_ChatWindow = nullptr;
    }
    
    // 清理主窗口
    if (g_MainWindow) {
        glfwMakeContextCurrent(g_MainWindow);
        Csm::CubismFramework::Dispose();
        glfwDestroyWindow(g_MainWindow);
        g_MainWindow = nullptr;
    }
    
    glfwTerminate();
    
    std::cout << "[App] Cleanup complete" << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << " AIPet - Dual Window Mode " << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (!InitializeMainWindow()) {
        std::cerr << "[Error] Failed to initialize main window" << std::endl;
        return -1;
    }
    
    if (!InitializeChatWindow()) {
        std::cerr << "[Error] Failed to initialize chat window" << std::endl;
        return -1;
    }
    
    if (!InitializeLive2D()) {
        std::cerr << "[Error] Failed to initialize Live2D" << std::endl;
        Cleanup();
        return -1;
    }
    
    if (!InitializeAI()) {
        std::cerr << "[Error] Failed to initialize AI" << std::endl;
        Cleanup();
        return -1;
    }
    
    Run();
    Cleanup();
    
    return 0;
}