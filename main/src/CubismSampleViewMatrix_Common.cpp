/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismSampleViewMatrix_Common.hpp"
#include <math.h>
#include "LAppDefine.hpp"

CubismSampleViewMatrix_Common::CubismSampleViewMatrix_Common(Csm::CubismMatrix44*& deviceToScreen, int windowWidth, int windowHeight)
    : CubismViewMatrix()
{
    if (windowWidth == 0 || windowHeight == 0)
    {
        return;
    }

    // 以高度（竖直尺寸）为基准
    float ratio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    float left = -ratio;
    float right = ratio;
    float bottom = LAppDefine::ViewLogicalLeft;
    float top = LAppDefine::ViewLogicalRight;

    // 设置与设备对应的屏幕范围
    SetScreenRect(left, right, bottom, top);

    if (windowWidth > windowHeight)
    {
        float screenW = fabsf(right - left);
    // 相对于设备设置矩阵的缩放比例
        deviceToScreen->ScaleRelative(screenW / windowWidth, -screenW / windowWidth);
    }
    else
    {
        float screenH = fabsf(top - bottom);
    // 相对于设备设置矩阵的缩放比例
        deviceToScreen->ScaleRelative(screenH / windowHeight, -screenH / windowHeight);
    }

    // 将矩阵位置平移到以左上为原点的位置
    deviceToScreen->TranslateRelative(-windowWidth * 0.5f, -windowHeight * 0.5f);

    // 设置缩放率
    Scale(LAppDefine::ViewScale, LAppDefine::ViewScale);

    // 设置最大缩放率
    SetMaxScale(LAppDefine::ViewMaxScale);

    // 设置最小缩放率
    SetMinScale(LAppDefine::ViewMinScale);

    // 设置在与设备对应的逻辑坐标系上的可移动范围
    SetMaxScreenRect(
        LAppDefine::ViewLogicalMaxLeft,
        LAppDefine::ViewLogicalMaxRight,
        LAppDefine::ViewLogicalMaxBottom,
        LAppDefine::ViewLogicalMaxTop
    );
}

CubismSampleViewMatrix_Common::~CubismSampleViewMatrix_Common()
{
}
