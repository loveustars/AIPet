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
#include <regex>

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

// 初始提示词（方便在文件顶部快速编辑）
static std::string g_AIInitPrompt =
    "Please include an emotion tag in brackets at the end of your reply (e.g. [happy] or [F05]). "
    "Reply only once to acknowledge this instruction so the client can proceed.";

// ImGui 字体
static ImFont* g_ChineseFont = nullptr;

// 保存被 ImGui_ImplGlfw 安装的先前回调（若存在），以便我们在设置自定义回调时转发事件
static GLFWkeyfun g_prevChatKeyCallback = nullptr;
static GLFWcharfun g_prevChatCharCallback = nullptr;
static GLFWmousebuttonfun g_prevChatMouseButtonCallback = nullptr;
static GLFWcursorposfun g_prevChatCursorPosCallback = nullptr;
static GLFWscrollfun g_prevChatScrollCallback = nullptr;

// AI priming flags
static std::atomic<bool> g_AIPriming(false);
static std::atomic<bool> g_AIReady(false);

// 窗口关闭标志
static std::atomic<bool> g_ShouldClose(false);

// 模型配置
static const char* MODEL_NAME = "Haru";

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
    // 鼠标滚轮回调（用于模型缩放）
    glfwSetScrollCallback(g_MainWindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        EventHandler::OnScroll(window, xoffset, yoffset);
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
    
    // 回调：保留并转发 ImGui/GLFW 之前设置的回调，避免覆盖导致 IME 与文本输入异常
    // 键盘
    g_prevChatKeyCallback = glfwSetKeyCallback(g_ChatWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (g_prevChatKeyCallback) g_prevChatKeyCallback(window, key, scancode, action, mods);
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            g_ShouldClose = true;
        }
    });
    // 字符（IME 需要）
    g_prevChatCharCallback = glfwSetCharCallback(g_ChatWindow, [](GLFWwindow* window, unsigned int c) {
        if (g_prevChatCharCallback) g_prevChatCharCallback(window, c);
    });
    // 鼠标按键
    g_prevChatMouseButtonCallback = glfwSetMouseButtonCallback(g_ChatWindow, [](GLFWwindow* window, int button, int action, int mods) {
        if (g_prevChatMouseButtonCallback) g_prevChatMouseButtonCallback(window, button, action, mods);
    });
    // 光标移动
    g_prevChatCursorPosCallback = glfwSetCursorPosCallback(g_ChatWindow, [](GLFWwindow* window, double xpos, double ypos) {
        if (g_prevChatCursorPosCallback) g_prevChatCursorPosCallback(window, xpos, ypos);
    });
    // 滚轮
    g_prevChatScrollCallback = glfwSetScrollCallback(g_ChatWindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        if (g_prevChatScrollCallback) g_prevChatScrollCallback(window, xoffset, yoffset);
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
    // 向大模型发送一次性初始提示，要求它以后在回复中附带方括号情绪标记。
    // 设定 priming 标志为 true，直到收到模型的首次确认回复为止。
    g_AIPriming = true;
    g_AIReady = false;
    // 使用顶部可编辑的初始提示词，便于快速修改
    g_AIManager->sendMessage(g_AIInitPrompt);
    g_ChatHistory.push_back({"System", "Welcome! Dual-window AI Desktop Pet."});
    g_ChatHistory.push_back({"System", "Live2D window: Transparent background with decorations."});
    g_ChatHistory.push_back({"System", "Press ESC in any window to exit."});
    return true;

}

/**
 * @brief 渲染主窗口（Live2D）
 * 
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
        
        if (g_AIPriming) {
            g_ChatHistory.push_back({"System", "AI 正在准备，请稍候..."});
        } else {
            if (g_AIManager && !g_AIManager->isBusy()) {
                g_AIManager->sendMessage(input);
            } else {
                g_ChatHistory.push_back({"System", "AI is busy..."});
            }
        }
        
        memset(g_InputBuffer, 0, sizeof(g_InputBuffer));
    }

    // ===== 表情调试面板 =====
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Expressions (debug)")) {
        if (g_UserModel) {
            auto exprs = g_UserModel->GetExpressionNames();
            ImGui::Text("Loaded expressions: %zu", exprs.size());
            ImGui::BeginChild("ExprList", ImVec2(0, 120), true);
            int col = 0;
            for (const auto &ename : exprs) {
                ImGui::PushID(ename.c_str());
                if (ImGui::Button(ename.c_str())) {
                    g_UserModel->SetExpressionByName(ename);
                }
                ImGui::PopID();
                ++col;
                if (col % 3 != 0) ImGui::SameLine();
                else ImGui::NewLine();
            }
            ImGui::EndChild();
        } else {
            ImGui::TextDisabled("No model loaded");
        }
    }

    // 手动加载文件面板（用于模型或字体加载失败时的手工选择）
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Manual file loader")) {
        static bool showModelPopup = false;
        static bool showFontPopup = false;
        static char modelDirBuf[512] = "";
        static char modelFileBuf[256] = ""; // e.g. Haru.model3.json
        static char fontPathBuf[512] = "";

        ImGui::TextWrapped("If model or font failed to load, enter paths here and press Load.");
        if (ImGui::Button("Manual Load Model")) showModelPopup = true;
        ImGui::SameLine();
        if (ImGui::Button("Manual Load Font")) showFontPopup = true;

        if (showModelPopup) ImGui::OpenPopup("LoadModelPopup");
        if (ImGui::BeginPopupModal("LoadModelPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Model Directory", modelDirBuf, sizeof(modelDirBuf));
            ImGui::InputText("Model filename (e.g. Haru.model3.json)", modelFileBuf, sizeof(modelFileBuf));
            if (ImGui::Button("Load")) {
                // 尝试加载模型（在主 OpenGL 上下文）
                std::string dir = std::string(modelDirBuf);
                std::string file = std::string(modelFileBuf);
                if (!dir.empty() && !file.empty()) {
                    try {
                        // 切换上下文并重建模型
                        glfwMakeContextCurrent(g_MainWindow);
                        if (g_UserModel) {
                            g_UserModel->DeleteRenderer();
                            delete g_UserModel;
                            g_UserModel = nullptr;
                        }
                        g_CurrentModelDirectory = dir;
                        g_UserModel = new CubismUserModelExtend(std::string(MODEL_NAME), g_CurrentModelDirectory);
                        g_UserModel->LoadAssets(file.c_str());
                        MouseActionManager::GetInstance()->SetUserModel(g_UserModel);
                        g_ChatHistory.push_back({"System", "Model loaded successfully."});
                    } catch (const std::exception &e) {
                        g_ChatHistory.push_back({"System", std::string("Model load failed: ") + e.what()});
                    }
                } else {
                    g_ChatHistory.push_back({"System", "Model directory or filename empty."});
                }
                showModelPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) { showModelPopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (showFontPopup) ImGui::OpenPopup("LoadFontPopup");
        if (ImGui::BeginPopupModal("LoadFontPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Font file path", fontPathBuf, sizeof(fontPathBuf));
            if (ImGui::Button("Load")) {
                std::string fpath = std::string(fontPathBuf);
                if (!fpath.empty()) {
                    try {
                        ImGuiIO& io = ImGui::GetIO();
                        ImFont* font = io.Fonts->AddFontFromFileTTF(fpath.c_str(), 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
                        if (font) {
                            // 重新创建设备纹理对象以确保字体生效（在新 imgui 后端里使用 CreateDeviceObjects）
                            io.Fonts->Build();
                            ImGui_ImplOpenGL3_DestroyDeviceObjects();
                            ImGui_ImplOpenGL3_CreateDeviceObjects();
                            g_ChineseFont = font;
                            g_ChatHistory.push_back({"System", "Font loaded successfully."});
                        } else {
                            g_ChatHistory.push_back({"System", "Font load failed (AddFontFromFileTTF returned null)."});
                        }
                    } catch (const std::exception &e) {
                        g_ChatHistory.push_back({"System", std::string("Font load failed: ") + e.what()});
                    }
                } else {
                    g_ChatHistory.push_back({"System", "Font path empty."});
                }
                showFontPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) { showFontPopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
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
        
        // 渲染两个窗口
        RenderMainWindow();
        RenderChatWindow();
        
        // 处理事件（先处理用户输入，这样新发送的消息能在本循环后由 AI 返回）
        glfwPollEvents();

        // 检查 AI 响应并处理可能的方括号情绪标记（例如: "I am happy [happy]" 或 "今天天气很好 [高兴]"）
        if (g_AIManager) {
            std::string response = g_AIManager->getResult();
            if (!response.empty()) {
                // Always print raw AI reply to terminal for debugging
                std::cout << "[AI RAW] " << response << std::endl;
            }
            if (!response.empty()) {
                // 提取所有方括号内的标记
                std::vector<std::string> emotions;
                try {
                    std::regex re("\\[([^\\]]+)\\]");
                    std::smatch m;
                    std::string tmp = response;
                    while (std::regex_search(tmp, m, re)) {
                        if (m.size() > 1) {
                            emotions.push_back(m[1].str());
                        }
                        tmp = m.suffix().str();
                    }

                    // 从显示文本中移除所有方括号标记
                    std::string cleaned = std::regex_replace(response, re, "");
                    // trim 前后空白
                    auto ltrim = [](std::string &s) {
                        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                    };
                    auto rtrim = [](std::string &s) {
                        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
                    };
                    ltrim(cleaned);
                    rtrim(cleaned);

                    // 如果去掉标记后没有可显示文本，则显示一个占位文本（不显示情绪标记本身）
                    if (cleaned.empty()) cleaned = "(expressed emotion)";

                    // 如果当前处于 priming 阶段，则隐藏该回复并把它视为初始化确认
                    if (g_AIPriming) {
                        // 在界面上提示用户 AI 已准备好
                        g_ChatHistory.push_back({"System", "AI 已准备好，你可以开始使用。"});
                        g_AIPriming = false;
                        g_AIReady = true;
                        // 仍然使用提取到的情绪去触发表情，但不展示模型的原始文本
                        if (g_UserModel) {
                            for (const auto &emo : emotions) {
                                std::string t = emo;
                                auto trim = [](std::string &s) {
                                    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                                    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
                                };
                                trim(t);
                                if (t.empty()) continue;
                                if ((t.size() == 3 || t.size() == 4) && (t[0] == 'F' || t[0] == 'f')) {
                                    std::string up = t;
                                    for (auto &c : up) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
                                    g_UserModel->SetExpressionByName(up);
                                } else {
                                    g_UserModel->SetExpressionByAIText(t);
                                }
                            }
                        }
                    } else {
                        g_ChatHistory.push_back({"AI", cleaned});

                        // 将提取的情绪用于触发表情：若是类似 F01/F02 的代码则直接按名触发，否则按文本映射触发
                        if (g_UserModel) {
                            for (const auto &emo : emotions) {
                                // 去除可能的空白
                                std::string t = emo;
                                auto trim = [](std::string &s) {
                                    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                                    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
                                };
                                trim(t);
                                if (t.empty()) continue;

                                // 如果是 Fxx 格式，直接按名触发
                                if ((t.size() == 3 || t.size() == 4) && (t[0] == 'F' || t[0] == 'f')) {
                                    // 规范化为大写 Fxx
                                    std::string up = t;
                                    for (auto &c : up) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
                                    g_UserModel->SetExpressionByName(up);
                                } else {
                                    // 作为文本让模型根据关键词/文本判断（支持中文/英文）
                                    g_UserModel->SetExpressionByAIText(t);
                                }
                            }
                        }
                    }
                } catch (const std::exception &e) {
                    // 解析正则发生问题时，回退到原始行为：显示原文并根据全文触发表情
                    g_ChatHistory.push_back({"AI", response});
                    if (g_UserModel) g_UserModel->SetExpressionByAIText(response);
                }
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