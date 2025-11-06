// src/Live2DManager.hpp

#ifndef LIVE2D_MANAGER_HPP
#define LIVE2D_MANAGER_HPP

#include <string>
#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <ICubismAllocator.hpp>

// 前向声明，避免在头文件中包含过多内容
class WindowManager;

class Live2DManager {
public:
    Live2DManager();
    ~Live2DManager();

    // 初始化 Cubism 框架
    bool initialize(WindowManager* windowManager);

    // 从指定目录加载模型
    bool loadModel(const std::string& modelDir);

    // 每帧更新
    void update();

    // 每帧绘制
    void draw();

    // 释放资源
    void cleanup();

private:
    Csm::CubismUserModel* _userModel;
    Csm::ICubismAllocator* _allocator;
    WindowManager* _windowManager; // 用于获取窗口尺寸等信息
};

#endif // LIVE2D_MANAGER_HPP