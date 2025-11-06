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
#include "stb_image.h"

// --- Cubism SDK 5 的文件加载辅助函数 ---

// 读取文件为字节数组
Csm::csmByte* LoadFileAsBytes(const Csm::csmChar* filePath, Csm::csmSizeInt* outSize) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
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
    : _userModel(nullptr), _allocator(new LAppAllocator()), _windowManager(nullptr) {}

Live2DManager::~Live2DManager() {
    cleanup();
    delete _allocator;
}

bool Live2DManager::initialize(WindowManager* windowManager) {
    _windowManager = windowManager;

    // Cubism SDK 5 的初始化选项
    Csm::CubismFramework::Option option;
    option.LogFunction = [](const Csm::csmChar* message) { 
        std::cout << "[Live2D] " << message << std::endl; 
    };
    option.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;

    if (!Csm::CubismFramework::StartUp(_allocator, &option)) {
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

    // 创建渲染器
    _userModel->CreateRenderer();
    
    // 加载并绑定纹理
    Csm::Rendering::CubismRenderer_OpenGLES2* renderer = 
        *(_userModel->GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2*>());
    
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

    std::cout << "[Live2D] Model loaded successfully with " << _textureIds.size() << " textures." << std::endl;
    return true;
}

void Live2DManager::update() {
    if (!_userModel || !_userModel->GetModel()) return;
    
    // 获取时间增量
    static Csm::csmFloat32 lastTime = 0.0f;
    Csm::csmFloat32 currentTime = SDL_GetTicks() / 1000.0f;
    Csm::csmFloat32 deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    
    // 更新 CubismModel（注意这里调用的是 GetModel()->Update()）
    _userModel->GetModel()->Update();
    
    // 更新物理
    /*
    if (_userModel->GetPhysics()) {
        _userModel->GetPhysics()->Evaluate(_userModel->GetModel(), deltaTime);
    }
    
    // 更新姿态
    if (_userModel->GetPose()) {
        _userModel->GetPose()->UpdateParameters(_userModel->GetModel(), deltaTime);
    }*/
}

void Live2DManager::draw() {
    if (!_userModel || !_userModel->GetModel()) return;

    int width, height;
    _windowManager->getWindowSize(width, height);
    if (width == 0 || height == 0) return;

    // 保存 OpenGL 状态
    GLint lastProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
    GLint lastTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
    GLboolean lastBlend = glIsEnabled(GL_BLEND);
    
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 计算投影矩阵
    Csm::CubismMatrix44 projection;
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    projection.Scale(1.0f, aspect);
    
    // 模型变换矩阵
    Csm::CubismMatrix44 modelMatrix;
    modelMatrix.Translate(0.0f, -0.5f);  // 调整位置
    modelMatrix.Scale(2.0f, 2.0f);       // 调整大小
    
    // 合并矩阵
    projection.MultiplyByMatrix(&modelMatrix);
    
    // 获取渲染器并绘制
    Csm::Rendering::CubismRenderer_OpenGLES2* renderer = 
        *(_userModel->GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2*>());
    
    if (renderer) {
        renderer->SetMvpMatrix(&projection);
        renderer->DrawModel();
    }
    
    // 恢复 OpenGL 状态
    glUseProgram(lastProgram);
    glBindTexture(GL_TEXTURE_2D, lastTexture);
    if (!lastBlend) glDisable(GL_BLEND);
}

void Live2DManager::cleanup() {
    // 删除纹理
    if (!_textureIds.empty()) {
        glDeleteTextures(_textureIds.size(), _textureIds.data());
        _textureIds.clear();
    }
    
    // 删除模型
    if (_userModel) {
        _userModel->DeleteRenderer();
        CSM_DELETE(_userModel);
        _userModel = nullptr;
    }
    
    // 清理框架
    Csm::CubismFramework::Dispose();
    std::cout << "[Live2D] Cleanup completed." << std::endl;
}