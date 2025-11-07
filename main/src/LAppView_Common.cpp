/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppView_Common.hpp"

#include <math.h>

#include "LAppDefine.hpp"

LAppView_Common::LAppView_Common()
{
    // 用于将设备坐标转换为屏幕坐标的矩阵
    _deviceToScreen = new Csm::CubismMatrix44();

    // 用于处理屏幕显示的缩放与平移变换的视图矩阵
    _viewMatrix = new Csm::CubismViewMatrix();
}

LAppView_Common::~LAppView_Common()
{
    delete _viewMatrix;
    delete _deviceToScreen;
}

void LAppView_Common::Initialize(int width, int height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    // 以高度（竖直尺寸）为基准计算纵横比
    float ratio = static_cast<float>(width) / static_cast<float>(height);
    float left = -ratio;
    float right = ratio;
    float bottom = LAppDefine::ViewLogicalLeft;
    float top = LAppDefine::ViewLogicalRight;

    _viewMatrix->SetScreenRect(left, right, bottom, top); // 设置与设备对应的屏幕范围：X左端, X右端, Y下端, Y上端
    _viewMatrix->Scale(LAppDefine::ViewScale, LAppDefine::ViewScale);

    _deviceToScreen->LoadIdentity(); // 当尺寸改变时需要重置
    if (width > height)
    {
        float screenW = fabsf(right - left);
        // 相对于设备设置矩阵的缩放比例（横向变换）
        _deviceToScreen->ScaleRelative(screenW / width, -screenW / width);
    }
    else
    {
        float screenH = fabsf(top - bottom);
        // 相对于设备设置矩阵的缩放比例（纵向变换）
        _deviceToScreen->ScaleRelative(screenH / height, -screenH / height);
    }
    _deviceToScreen->TranslateRelative(-width * 0.5f, -height * 0.5f);

    // 设置视图缩放范围
    _viewMatrix->SetMaxScale(LAppDefine::ViewMaxScale); // 最大缩放比例
    _viewMatrix->SetMinScale(LAppDefine::ViewMinScale); // 最小缩放比例

    // 设置视图在逻辑坐标系中可移动的最大范围
    _viewMatrix->SetMaxScreenRect(
        LAppDefine::ViewLogicalMaxLeft,
        LAppDefine::ViewLogicalMaxRight,
        LAppDefine::ViewLogicalMaxBottom,
        LAppDefine::ViewLogicalMaxTop
    );
}

float LAppView_Common::TransformViewX(float deviceX) const
{
    float screenX = _deviceToScreen->TransformX(deviceX); // 获取逻辑坐标变换后的 X 值。
    return _viewMatrix->InvertTransformX(screenX); // 应用视图的缩放/平移后的坐标。
}

float LAppView_Common::TransformViewY(float deviceY) const
{
    float screenY = _deviceToScreen->TransformY(deviceY); // 获取逻辑坐标变换后的 Y 值。
    return _viewMatrix->InvertTransformY(screenY); // 应用视图的缩放/平移后的坐标。
}

float LAppView_Common::TransformScreenX(float deviceX) const
{
    return _deviceToScreen->TransformX(deviceX);
}

float LAppView_Common::TransformScreenY(float deviceY) const
{
    return _deviceToScreen->TransformY(deviceY);
}
