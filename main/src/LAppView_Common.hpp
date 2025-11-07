/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <Math/CubismMatrix44.hpp>
#include <Math/CubismViewMatrix.hpp>

#include "CubismFramework.hpp"

/**
 * @brief 渲染视图类
 */
class LAppView_Common
{
public:
    /**
     * @brief 构造函数
     */
    LAppView_Common();

    /**
    * @brief 析构函数
    */
    virtual ~LAppView_Common();

    /**
    * @brief 初始化视图矩阵和设备到屏幕的变换
    */
    virtual void Initialize(int width, int height);

    /**
    * @brief 将设备坐标的 X 转换为视图坐标
    *
    * @param[in] deviceX 设备 X 坐标
    */
    virtual float TransformViewX(float deviceX) const;

    /**
    * @brief 将设备坐标的 Y 转换为视图坐标
    *
    * @param[in] deviceY 设备 Y 坐标
    */
    virtual float TransformViewY(float deviceY) const;

    /**
    * @brief 将设备坐标的 X 转换为屏幕坐标
    *
    * @param[in] deviceX 设备 X 坐标
    */
    virtual float TransformScreenX(float deviceX) const;

    /**
    * @brief 将设备坐标的 Y 转换为屏幕坐标
    *
    * @param[in] deviceY 设备 Y 坐标
    */
    virtual float TransformScreenY(float deviceY) const;

protected:
    Csm::CubismMatrix44* _deviceToScreen;    ///< 从设备坐标转换到屏幕坐标的矩阵
    Csm::CubismViewMatrix* _viewMatrix;      ///< viewMatrix
};
