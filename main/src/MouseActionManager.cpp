/**
* Copyright(c) Live2D Inc. All rights reserved.
*
* Use of this source code is governed by the Live2D Open Software license
* that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
*/

#include "MouseActionManager.hpp"
#include <iostream>

namespace {
    MouseActionManager* instance = NULL;
}

MouseActionManager* MouseActionManager::GetInstance()
{
    if (!instance)
    {
        instance = new MouseActionManager();
    }

    return instance;
}

void MouseActionManager::ReleaseInstance()
{
    if (instance)
    {
        delete instance;
    }

    instance = NULL;
}

MouseActionManager::MouseActionManager() : MouseActionManager_Common()
{
    _middleCaptured = false;
    _lastMouseX = 0.0f;
    _lastMouseY = 0.0f;
}

MouseActionManager::~MouseActionManager()
{
}

void MouseActionManager::OnMouseCallBack(GLFWwindow* window, int button, int action, int modify)
{
    // 支持左键用于模型拖拽（原有行为）以及中键用于视图平移
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        switch (action)
        {
        case GLFW_PRESS:
            _captured = true;
            OnTouchesBegan(_mouseX, _mouseY);
            break;
        case GLFW_RELEASE:
            if (_captured)
            {
                _captured = false;
                OnTouchesEnded(_mouseX, _mouseY);
            }
            break;
        default:
            break;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        // 中键按下开始平移视图
        switch (action)
        {
        case GLFW_PRESS:
            _middleCaptured = true;
            _lastMouseX = _mouseX;
            _lastMouseY = _mouseY;
            std::cout << "[Mouse] Middle button pressed - start panning at (" << _lastMouseX << ", " << _lastMouseY << ")" << std::endl;
            break;
        case GLFW_RELEASE:
            _middleCaptured = false;
            std::cout << "[Mouse] Middle button released - stop panning" << std::endl;
            break;
        default:
            break;
        }
    }
}

void MouseActionManager::OnMouseCallBack(GLFWwindow* window, double x, double y)
{
    float newX = static_cast<float>(x);
    float newY = static_cast<float>(y);

    // 更新鼠标位置
    _mouseX = newX;
    _mouseY = newY;

    // 中键平移视图
    if (_middleCaptured && _viewMatrix && _deviceToScreen)
    {
        // 计算设备坐标的偏移并转换为视图坐标偏移
        float prevScreenX = _deviceToScreen->TransformX(_lastMouseX);
        float prevScreenY = _deviceToScreen->TransformY(_lastMouseY);
        float curScreenX = _deviceToScreen->TransformX(_mouseX);
        float curScreenY = _deviceToScreen->TransformY(_mouseY);

        float prevViewX = _viewMatrix->InvertTransformX(prevScreenX);
        float prevViewY = _viewMatrix->InvertTransformY(prevScreenY);
        float curViewX = _viewMatrix->InvertTransformX(curScreenX);
        float curViewY = _viewMatrix->InvertTransformY(curScreenY);

        float dx = curViewX - prevViewX;
        float dy = curViewY - prevViewY;

        _viewMatrix->AdjustTranslate(dx, dy);

        std::cout << "[Mouse] Panning view by (" << dx << ", " << dy << ")" << std::endl;
        _lastMouseX = _mouseX;
        _lastMouseY = _mouseY;
        return;
    }

    if (!_captured)
    {
        return;
    }

    OnTouchesMoved(_mouseX, _mouseY);
}

void MouseActionManager::OnScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    // yoffset > 0 表示向上滚动（放大），<0 表示缩小
    if (!_viewMatrix || !_deviceToScreen)
    {
        return;
    }

    // 将滚轮缩放中心设置为当前鼠标位置
    float screenX = _deviceToScreen->TransformX(_mouseX);
    float screenY = _deviceToScreen->TransformY(_mouseY);
    float viewX = _viewMatrix->InvertTransformX(screenX);
    float viewY = _viewMatrix->InvertTransformY(screenY);

    // 缩放因子：每个刻度缩放 1.1 倍
    float factor = 1.0f + static_cast<float>(yoffset) * 0.5f;
    if (factor <= 0.0f) factor = 1.0f;

    std::cout << "[Mouse] Scroll detected yoffset=" << yoffset << " factor=" << factor << std::endl;
    _viewMatrix->AdjustScale(viewX, viewY, factor);
}
