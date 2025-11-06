// src/Live2DManager.hpp

#ifndef LIVE2D_MANAGER_HPP
#define LIVE2D_MANAGER_HPP

#include <string>
#include <vector>
#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <ICubismAllocator.hpp>
#include <GL/glew.h>

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

    void setModelScale(float scale) { _modelScale = scale; }
    void setModelPosition(float x, float y) { _modelX = x; _modelY = y; }
    float getModelScale() const { return _modelScale; }

private:
    Csm::CubismUserModel* _userModel;
    Csm::ICubismAllocator* _allocator;
    WindowManager* _windowManager;
    std::vector<GLuint> _textureIds;  // 存储加载的纹理 ID
    bool _modelLoaded;
    Csm::CubismFramework::Option _cubismOption;
    // 新增成员变量
    float _modelScale = 1.8f;
    float _modelX = 0.0f;
    float _modelY = -0.2f;
};

#endif // LIVE2D_MANAGER_HPP