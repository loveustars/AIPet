/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "MouseActionManager_Common.hpp"

#include "LAppDefine.hpp"

namespace {
    MouseActionManager_Common* instance = NULL;
}

MouseActionManager_Common* MouseActionManager_Common::GetInstance()
{
    if (instance == NULL)
    {
        instance = new MouseActionManager_Common();
    }

    return instance;
}

void MouseActionManager_Common::ReleaseInstance()
{
    if (instance != NULL)
    {
        delete instance;
    }

    instance = NULL;
}

MouseActionManager_Common::MouseActionManager_Common()
{
}

MouseActionManager_Common::~MouseActionManager_Common()
{
    // 释放矩阵数据
    delete _viewMatrix;

    // 释放触摸管理器
    delete _TouchManager;
}

void MouseActionManager_Common::Initialize(int windowWidth, int windowHeight)
{
    // 初始化视图矩阵
    ViewInitialize(windowWidth, windowHeight);

    // 创建触摸事件管理器
    _TouchManager = new TouchManager_Common();

    _captured = false;
    _mouseX = 0.0f;
    _mouseY = 0.0f;
}

void MouseActionManager_Common::ViewInitialize(int windowWidth, int windowHeight)
{
    _deviceToScreen = new Csm::CubismMatrix44();
    _viewMatrix = new CubismSampleViewMatrix_Common(_deviceToScreen, windowWidth, windowHeight);
}

void MouseActionManager_Common::OnDrag(Csm::csmFloat32 x, Csm::csmFloat32 y)
{
    _userModel->SetDragging(x, y);
}

void MouseActionManager_Common::OnTouchesBegan(float px, float py)
{
    _TouchManager->TouchesBegan(px, py);
}

void MouseActionManager_Common::OnTouchesMoved(float px, float py)
{
    float screenX = _deviceToScreen->TransformX(_TouchManager->GetX()); // 将设备/窗口坐标转换到逻辑屏幕坐标。
    float viewX = _viewMatrix->InvertTransformX(screenX); // 应用视图的缩放/平移得到视图坐标。

    float screenY = _deviceToScreen->TransformY(_TouchManager->GetY()); // 将设备/窗口坐标转换到逻辑屏幕坐标。
    float viewY = _viewMatrix->InvertTransformY(screenY); // 应用视图的缩放/平移得到视图坐标。

    _TouchManager->TouchesMoved(px, py);

    // 设置拖拽信息到模型
    _userModel->SetDragging(viewX, viewY);
}

void MouseActionManager_Common::OnTouchesEnded(float px, float py)
{
    // 触摸/点击结束，清除拖拽信息
    OnDrag(0.0f, 0.0f);
}

CubismSampleViewMatrix_Common * MouseActionManager_Common::GetViewMatrix()
{
    return _viewMatrix;
}

void MouseActionManager_Common::SetUserModel(Csm::CubismUserModel * userModel)
{
    _userModel = userModel;
}
