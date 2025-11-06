// src/Live2DManager.cpp

#include "Live2DManager.hpp"
#include "WindowManager.hpp"
#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <CubismModelSettingJson.hpp>
#include <Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp>
#include <Motion/CubismMotion.hpp>
#include <Id/CubismIdManager.hpp>
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

// STB Image library for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// --- Cubism SDK 5 的文件加载辅助函数 ---

// 读取文件为字节数组
Csm::csmByte* LoadFileAsBytes(std::string filePath, Csm::csmSizeInt* outSize) {
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[Live2D] File not found: " << filePath << std::endl;
        *outSize = 0;
        return nullptr;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    Csm::csmByte* buffer = static_cast<Csm::csmByte*>(CSM_MALLOC(size));
    if (!file.read(reinterpret_cast<char*>(buffer), size)) {
        CSM_FREE(buffer);
        *outSize = 0;
        return nullptr;
    }
    
    *outSize = static_cast<Csm::csmSizeInt>(size);
    file.close();
    return buffer;
}


// 释放字节数组
void ReleaseFileAsBytes(Csm::csmByte* byteData) {
    CSM_FREE(byteData);
}

// --- 纹理加载函数 ---
GLuint LoadTexture(const char* filePath) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filePath, &width, &height, &channels, 4); // 强制加载为 RGBA
    
    if (!data) {
        std::cerr << "[Live2D] Failed to load texture: " << filePath << std::endl;
        std::cerr << "[Live2D] STB Error: " << stbi_failure_reason() << std::endl;
        return 0;
    }
    
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 上传纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    
    std::cout << "[Live2D] Texture loaded successfully: " << filePath 
              << " (ID: " << textureId << ", " << width << "x" << height << ")" << std::endl;
    
    return textureId;
}

// --- 自定义内存分配器 (仅 Linux) ---
class LAppAllocator : public Csm::ICubismAllocator {
public:
    void* Allocate(Csm::csmSizeType size) override { 
        return malloc(size); 
    }
    
    void Deallocate(void* memory) override { 
        free(memory); 
    }
    
    void* AllocateAligned(Csm::csmSizeType size, Csm::csmUint32 alignment) override { 
        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return nullptr;
        }
        return ptr;
    }
    
    void DeallocateAligned(void* alignedMemory) override {
        free(alignedMemory);
    }
};

// --- Live2DManager 实现 ---
Live2DManager::Live2DManager() 
    : _userModel(nullptr), _allocator(new LAppAllocator()), _windowManager(nullptr), _modelLoaded(false) {}

Live2DManager::~Live2DManager() {
    cleanup();
    delete _allocator;
}

bool Live2DManager::initialize(WindowManager* windowManager) {
    _windowManager = windowManager;

    _cubismOption.LogFunction = [](const Csm::csmChar* message) { 
        std::cout << "[Live2D] " << message << std::endl; 
    };
    // --- 关键修正：移除重复的命名空间 ---
    // 正确的行
    _cubismOption.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;    
    // --- 现在这里的赋值是 100% 类型安全的 ---
    _cubismOption.LoadFileFunction = ::LoadFileAsBytes;
    _cubismOption.ReleaseBytesFunction = ::ReleaseFileAsBytes;

    if (!Csm::CubismFramework::StartUp(_allocator, &_cubismOption)) {
        std::cerr << "[Live2D] Failed to start up CubismFramework!" << std::endl;
        return false;
    }

    Csm::CubismFramework::Initialize();
    
    std::cout << "[Live2D] Framework initialized successfully." << std::endl;
    return true;
}

bool Live2DManager::loadModel(const std::string& modelDir) {
    if (_userModel) {
        _userModel->DeleteRenderer();
        CSM_DELETE(_userModel);
        _userModel = nullptr;
        _modelLoaded = false;
    }

    // 构建 model3.json 文件路径
    std::string modelJsonFileName = modelDir.substr(modelDir.find_last_of("/\\") + 1);
    modelJsonFileName += ".model3.json";
    std::string modelJsonPath = modelDir + "/" + modelJsonFileName;

    std::cout << "[Live2D] Loading model from: " << modelJsonPath << std::endl;

    // 加载 model3.json 文件
    Csm::csmSizeInt size = 0;
    Csm::csmByte* buffer = LoadFileAsBytes(modelJsonPath.c_str(), &size);
    if (!buffer || size == 0) {
        std::cerr << "[Live2D] Failed to load model JSON file." << std::endl;
        return false;
    }

    // 解析 model3.json
    Csm::ICubismModelSetting* setting = new Csm::CubismModelSettingJson(buffer, size);
    ReleaseFileAsBytes(buffer);

    // 创建 UserModel
    _userModel = CSM_NEW Csm::CubismUserModel();
    
    // 加载 .moc3 文件
    std::string mocFileName = setting->GetModelFileName();
    std::string mocFilePath = modelDir + "/" + mocFileName;
    buffer = LoadFileAsBytes(mocFilePath.c_str(), &size);
    if (!buffer) {
        std::cerr << "[Live2D] Failed to load .moc3 file: " << mocFilePath << std::endl;
        CSM_DELETE(_userModel);
        _userModel = nullptr;
        CSM_DELETE(setting);
        return false;
    }
    
    _userModel->LoadModel(buffer, size);
    ReleaseFileAsBytes(buffer);

    // 检查模型是否加载成功
    if (!_userModel->GetModel()) {
        std::cerr << "[Live2D] Failed to create model from .moc3 file" << std::endl;
        CSM_DELETE(_userModel);
        _userModel = nullptr;
        CSM_DELETE(setting);
        return false;
    }

    std::cout << "[Live2D] Model loaded, drawable count: " << _userModel->GetModel()->GetDrawableCount() << std::endl;

    // 加载表情数据
    for (Csm::csmInt32 i = 0; i < setting->GetExpressionCount(); i++) {
        std::string expName = setting->GetExpressionName(i);
        std::string expFileName = setting->GetExpressionFileName(i);
        std::string expFilePath = modelDir + "/" + expFileName;
        
        buffer = LoadFileAsBytes(expFilePath.c_str(), &size);
        if (buffer) {
            Csm::ACubismMotion* motion = _userModel->LoadExpression(buffer, size, expName.c_str());
            ReleaseFileAsBytes(buffer);
        }
    }

    // 加载物理文件
    if (strcmp(setting->GetPhysicsFileName(), "") != 0) {
        std::string physicsFileName = setting->GetPhysicsFileName();
        std::string physicsFilePath = modelDir + "/" + physicsFileName;
        
        buffer = LoadFileAsBytes(physicsFilePath.c_str(), &size);
        if (buffer) {
            _userModel->LoadPhysics(buffer, size);
            ReleaseFileAsBytes(buffer);
            std::cout << "[Live2D] Physics loaded successfully" << std::endl;
        }
    }

    // 加载姿态文件
    if (strcmp(setting->GetPoseFileName(), "") != 0) {
        std::string poseFileName = setting->GetPoseFileName();
        std::string poseFilePath = modelDir + "/" + poseFileName;
        
        buffer = LoadFileAsBytes(poseFilePath.c_str(), &size);
        if (buffer) {
            _userModel->LoadPose(buffer, size);
            ReleaseFileAsBytes(buffer);
            std::cout << "[Live2D] Pose loaded successfully" << std::endl;
        }
    }

    // 加载用户数据
    if (strcmp(setting->GetUserDataFile(), "") != 0) {
        std::string userDataFileName = setting->GetUserDataFile();
        std::string userDataFilePath = modelDir + "/" + userDataFileName;
        
        buffer = LoadFileAsBytes(userDataFilePath.c_str(), &size);
        if (buffer) {
            _userModel->LoadUserData(buffer, size);
            ReleaseFileAsBytes(buffer);
        }
    }

    // 创建渲染器 - 关键：必须在 OpenGL 上下文有效时创建
    std::cout << "[Live2D] Creating renderer..." << std::endl;
    _userModel->CreateRenderer();
    
    // 验证渲染器创建成功
    Csm::Rendering::CubismRenderer_OpenGLES2* renderer = 
        _userModel->GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2>();
    
    if (!renderer) {
        std::cerr << "[Live2D] Failed to create renderer!" << std::endl;
        CSM_DELETE(_userModel);
        _userModel = nullptr;
        CSM_DELETE(setting);
        return false;
    }
    
    std::cout << "[Live2D] Renderer created successfully" << std::endl;

    // 初始化渲染器 - 关键步骤！
    renderer->Initialize(_userModel->GetModel());
    
    // 加载并绑定纹理
    _textureIds.clear();
    for (Csm::csmInt32 i = 0; i < setting->GetTextureCount(); i++) {
        std::string texturePath = modelDir + "/" + setting->GetTextureFileName(i);
        GLuint textureId = LoadTexture(texturePath.c_str());
        
        if (textureId == 0) {
            std::cerr << "[Live2D] Warning: Failed to load texture " << i << std::endl;
        }
        
        _textureIds.push_back(textureId);
        renderer->BindTexture(i, textureId);
    }

    // 清理 setting
    CSM_DELETE(setting);

    // 标记模型已加载
    _modelLoaded = true;

    std::cout << "[Live2D] Model loaded successfully with " << _textureIds.size() << " textures." << std::endl;
    return true;
}

void Live2DManager::update() {
    if (!_modelLoaded || !_userModel || !_userModel->GetModel()) return;
    
    // 获取时间增量
    static Csm::csmFloat32 lastTime = 0.0f;
    Csm::csmFloat32 currentTime = SDL_GetTicks() / 1000.0f;
    Csm::csmFloat32 deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    
    // 更新 CubismModel
    _userModel->GetModel()->Update();
}


void Live2DManager::draw() {
    if (!_modelLoaded || !_userModel || !_userModel->GetModel()) {
        return;
    }

    int width, height;
    _windowManager->getWindowSize(width, height);
    if (width == 0 || height == 0) return;

    auto* renderer = _userModel->GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2>();
    if (!renderer) {
        return;
    }

    // --- 保存 OpenGL 状态 ---
    GLint lastProgram, lastTexture, lastArrayBuffer, lastElementArrayBuffer;
    glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &lastElementArrayBuffer);
    GLint lastViewport[4];
    glGetIntegerv(GL_VIEWPORT, lastViewport);
    GLboolean lastBlend = glIsEnabled(GL_BLEND);
    GLboolean lastCullFace = glIsEnabled(GL_CULL_FACE);
    GLboolean lastDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean lastScissorTest = glIsEnabled(GL_SCISSOR_TEST);
    
    // --- 设置 Live2D 需要的 OpenGL 状态 ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);

    // ===== 关键修复：正确计算 MVP 矩阵 =====
    
    // 1. 创建投影矩阵（将 Live2D 坐标系映射到 OpenGL 裁剪空间）
    Csm::CubismMatrix44 projection;
    projection.LoadIdentity();
    
    // 计算宽高比
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    
    // 2. 设置正交投影
    // Live2D 模型通常在 [-1, 1] 的范围内，我们需要根据窗口宽高比调整
    if (aspectRatio > 1.0f) {
        // 横屏：扩展宽度
        float viewWidth = aspectRatio;
        projection.Scale(1.0f / viewWidth, 1.0f);
    } else {
        // 竖屏：扩展高度
        float viewHeight = 1.0f / aspectRatio;
        projection.Scale(1.0f, 1.0f / viewHeight);
    }
    
    // 3. 获取模型矩阵并设置模型的位置和大小
    Csm::CubismModelMatrix* modelMatrix = _userModel->GetModelMatrix();
    
    // 重置模型矩阵的变换
    modelMatrix->LoadIdentity();
    
    // 设置模型的位置（X, Y）
    // X: 0.0 表示水平居中
    // Y: -0.2 表示稍微向下移动（根据需要调整）
    modelMatrix->SetPosition(_modelX, _modelY);
    
    // 设置模型缩放
    // 通常 Live2D 模型需要缩放到合适的大小
    // 1.8 是一个比较常用的缩放值，你可以根据实际效果调整
    float modelScale = 1.8f;
    modelMatrix->Scale(_modelScale, _modelScale);;
    
    // 4. 将投影矩阵和模型矩阵相乘
    projection.MultiplyByMatrix(modelMatrix);
    
    // 5. 设置最终的 MVP 矩阵到渲染器
    renderer->SetMvpMatrix(&projection);
    
    // --- 调试信息（每 60 帧打印一次）---
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {
        std::cout << "[Live2D] Frame " << frameCount 
                  << " | Viewport: " << width << "x" << height 
                  << " | Aspect: " << aspectRatio 
                  << " | Drawables: " << _userModel->GetModel()->GetDrawableCount()
                  << " | Scale: " << modelScale << std::endl;
    }
    
    // --- 绘制模型 ---
    renderer->DrawModel();
    
    // --- 恢复 OpenGL 状态 ---
    glUseProgram(lastProgram);
    glBindTexture(GL_TEXTURE_2D, lastTexture);
    glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lastElementArrayBuffer);
    
    if (lastBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (lastCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (lastDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (lastScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    
    glViewport(lastViewport[0], lastViewport[1], 
               (GLsizei)lastViewport[2], (GLsizei)lastViewport[3]);
}


void Live2DManager::cleanup() {
    std::cout << "[Live2D] Starting cleanup..." << std::endl;
    
    // 删除纹理
    if (!_textureIds.empty()) {
        glDeleteTextures(_textureIds.size(), _textureIds.data());
        _textureIds.clear();
        std::cout << "[Live2D] Textures deleted" << std::endl;
    }
    
    // 删除模型
    if (_userModel) {
        if (_userModel->GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2>()) {
            _userModel->DeleteRenderer();
            std::cout << "[Live2D] Renderer deleted" << std::endl;
        }
        CSM_DELETE(_userModel);
        _userModel = nullptr;
        std::cout << "[Live2D] Model deleted" << std::endl;
    }
    
    _modelLoaded = false;
    
    // 清理框架
    Csm::CubismFramework::Dispose();
    std::cout << "[Live2D] Framework disposed" << std::endl;
    std::cout << "[Live2D] Cleanup completed." << std::endl;
}