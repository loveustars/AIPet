// src/Live2DManager.cpp

#include "Live2DManager.hpp"
#include "WindowManager.hpp"
#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <CubismModelSettingJson.hpp>
#include <Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp>
#include <iostream>
#include <cstdlib> // For posix_memalign

// --- 辅助函数，适配新版 SDK 的文件加载机制 ---

// C-style function that matches the csmLoadFileFunction signature
Csm::csmByte* LoadFileAsBytes(const std::string filePath, Csm::csmSizeInt* outSize) {
    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) {
        std::cerr << "[Live2D] File not found: " << filePath << std::endl;
        *outSize = 0;
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    *outSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    Csm::csmByte* buf = static_cast<Csm::csmByte*>(CSM_MALLOC(*outSize));
    fread(buf, 1, *outSize, fp);
    fclose(fp);
    return buf;
}

// C-style function that matches the csmReleaseBytesFunction signature
void ReleaseFileAsBytes(Csm::csmByte* byteData) {
    CSM_FREE(byteData);
}

// --- 自定义内存分配器 (与之前相同) ---
class LAppAllocator : public Csm::ICubismAllocator {
public:
    void* Allocate(Csm::csmSizeType size) override { return malloc(size); }
    void Deallocate(void* memory) override { free(memory); }
    void* AllocateAligned(Csm::csmSizeType size, Csm::csmUint32 alignment) override { 
        void* ptr;
        posix_memalign(&ptr, alignment, size);
        return ptr;
    }
    void DeallocateAligned(void* memory) override { free(memory); }
};


// --- Live2DManager 实现 (适配新版 SDK) ---
Live2DManager::Live2DManager() 
    : _userModel(nullptr), _allocator(new LAppAllocator()), _windowManager(nullptr) {}

Live2DManager::~Live2DManager() {
    cleanup();
    delete _allocator;
}

bool Live2DManager::initialize(WindowManager* windowManager) {
    _windowManager = windowManager;

    Csm::CubismFramework::Option option;
    option.LogFunction = [](const char* message) { std::cout << "[Live2D] " << message << std::endl; };
    option.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;

    // --- 关键修改：将我们的文件加载函数注入到框架中 ---
    option.LoadFileFunction = ::LoadFileAsBytes;
    option.ReleaseBytesFunction = ::ReleaseFileAsBytes;

    if (Csm::CubismFramework::StartUp(_allocator, &option) == false) {
        std::cerr << "Failed to start up CubismFramework!" << std::endl;
        return false;
    }

    Csm::CubismFramework::Initialize();
    return true;
}

bool Live2DManager::loadModel(const std::string& modelDir) {
    if (_userModel) {
        CSM_DELETE(_userModel);
        _userModel = nullptr;
    }

    // --- 加载 .model3.json 文件 ---
    std::string modelJsonFileName = modelDir.substr(modelDir.find_last_of("/\\") + 1);
    modelJsonFileName += ".model3.json";
    std::string modelJsonPath = modelDir + "/" + modelJsonFileName;

    Csm::csmSizeInt size = 0;
    // 使用框架注入的函数来加载文件
    Csm::csmByte* buffer = Csm::CubismFramework::GetLoadFileFunction()(modelJsonPath, &size);
    if (!buffer) return false;

    // 使用 CubismModelSettingJson 解析文件
    Csm::CubismModelSettingJson* setting = new Csm::CubismModelSettingJson(buffer, size);
    // 释放 buffer
    Csm::CubismFramework::GetReleaseBytesFunction()(buffer);

    // --- 创建并设置模型 ---
    _userModel = CSM_NEW Csm::CubismUserModel();
    if (!_userModel->SetupModel(setting)) { // SetupModel 会接管 setting，不需要手动 delete
        CSM_DELETE(_userModel);
        _userModel = nullptr;
        std::cerr << "Failed to setup model from setting." << std::endl;
        return false;
    }

    // 创建渲染器
    _userModel->CreateRenderer();
    
    return true;
}

void Live2DManager::update() {
    if (!_userModel || !_userModel->GetModel()) return;
    
    // 获取时间增量
    float deltaTime = 0.016f; // 暂时固定为 60fps
    
    // 更新 UserModel，它会处理动画、物理等
    _userModel->Update(deltaTime);
}

void Live2DManager::draw() {
    if (!_userModel || !_userModel->GetModel()) return;

    int width, height;
    _windowManager->getWindowSize(width, height);
    if (width == 0 || height == 0) return;

    // 计算 MVP 矩阵
    Csm::CubismMatrix44 projection;
    projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height));
    
    Csm::CubismMatrix44 modelMatrix;
    // 设置模型的位置和大小
    modelMatrix.Translate(0.0f, -1.0f); // 移动到底部
    modelMatrix.Scale(2.0f, 2.0f);   // 缩放到合适大小

    projection.MultiplyByMatrix(&modelMatrix);
    
    // --- 关键修改：调用渲染器进行绘制 ---
    if (_userModel->GetRenderer()) {
        _userModel->GetRenderer()->SetMvpMatrix(&projection);
        _userModel->GetRenderer()->DrawModel();
    }
}

void Live2DManager::cleanup() {
    if (_userModel) {
        // --- 关键修改：使用 CSM_DELETE 宏 ---
        CSM_DELETE(_userModel);
        _userModel = nullptr;
    }
    // 框架清理
    Csm::CubismFramework::Dispose();
}