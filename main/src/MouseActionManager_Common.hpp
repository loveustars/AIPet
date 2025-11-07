/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once


#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <Math/CubismMatrix44.hpp>

#include "TouchManager_Common.hpp"
#include "CubismSampleViewMatrix_Common.hpp"

/**
 * @brief 通知鼠标操作的管理类
 *
 * 将鼠标（或触摸）输入桥接到 Cubism 框架。
 */
class MouseActionManager_Common
{
public:
    /**
     * @brief 返回类实例（单例）
     *
     * 若实例尚未生成则内部创建并返回。
     * @return 类实例指针
     */
    static MouseActionManager_Common* GetInstance();

    /**
     * @brief 释放单例实例
     */
    static void ReleaseInstance();

    MouseActionManager_Common(); ///< 构造函数
    virtual ~MouseActionManager_Common(); ///< 析构函数

    /**
     * @brief 执行必要的初始化
     */
    virtual void Initialize(int windowWidth, int windowHeight);

    /**
     * @brief 初始化视图矩阵
     */
    virtual void ViewInitialize(int windowWidth, int windowHeight);

    /**
     * @brief 设置要操作的用户模型指针
     */
    virtual void SetUserModel(Csm::CubismUserModel* userModel);

    /**
     * @brief 获取视图矩阵对象
     */
    virtual CubismSampleViewMatrix_Common* GetViewMatrix();

    /**
     * @brief 拖拽事件
     *
     * 在拖拽时通知移动的偏移量。
     */
    virtual void OnDrag(Csm::csmFloat32 x, Csm::csmFloat32 y);

    /**
     * @brief 点击/触摸开始
     *
     * 当点击或触摸开始时调用。
     */
    virtual void OnTouchesBegan(float px, float py);

    /**
     * @brief 在点击/触摸移动时调用
     */
    virtual void OnTouchesMoved(float px, float py);

    /**
     * @brief 点击/触摸结束
     *
     * 当点击或触摸结束时调用。
     */
    virtual void OnTouchesEnded(float px, float py);

protected:
    Csm::CubismUserModel* _userModel;

    TouchManager_Common* _TouchManager;                 ///< 触摸管理器

    bool _captured;                              ///< 是否处于按下/捕获状态
    float _mouseX;                               ///< 鼠标/触摸 X 坐标
    float _mouseY;                               ///< 鼠标/触摸 Y 坐标

    CubismSampleViewMatrix_Common* _viewMatrix; ///< 用于视图缩放与位移变换的矩阵
    Csm::CubismMatrix44* _deviceToScreen; ///< 从设备坐标到屏幕坐标的变换矩阵
};
