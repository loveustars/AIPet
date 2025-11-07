/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>
#include <Math/CubismViewMatrix.hpp>

/**
 * @brief 继承自 CubismViewMatrix 的类
 *
 * 对视图矩阵进行了包装，以便在 Cubism 中更方便地使用。
 *
 */
class CubismSampleViewMatrix_Common :
    public Csm::CubismViewMatrix
{
public:
    CubismSampleViewMatrix_Common(Csm::CubismMatrix44*& deviceToScreen, int windowWidth, int windowHeight); ///< 构造函数

    virtual ~CubismSampleViewMatrix_Common(); ///< 析构函数
};
