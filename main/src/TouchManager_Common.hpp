/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

class TouchManager_Common
{
public:
    TouchManager_Common();

    virtual float GetCenterX() const { return _lastX; }
    virtual float GetCenterY() const { return _lastY; }
    virtual float GetDeltaX() const { return _deltaX; }
    virtual float GetDeltaY() const{ return _deltaY; }
    virtual float GetStartX() const{ return _startX; }
    virtual float GetStartY() const{ return _startY; }
    virtual float GetScale() const { return _scale; }
    virtual float GetX() const{ return _lastX; }
    virtual float GetY() const{ return _lastY; }
    virtual float GetX1() const{ return _lastX1; }
    virtual float GetY1() const{ return _lastY1; }
    virtual float GetX2() const{ return _lastX2; }
    virtual float GetY2() const{ return _lastY2; }
    virtual float GetLastTouchDistance() const{ return _lastTouchDistance; }
    virtual bool IsSingleTouch() const { return _touchSingle; }
    virtual bool IsFlickAvailable() const { return _flipAvailable; }
    virtual void DisableFlick() { _flipAvailable = false; }

    /*
    * @brief 触摸开始事件
    *
    * @param[in] deviceY 屏幕上触摸的 y 值
    * @param[in] deviceX 屏幕上触摸的 x 值
    */
    virtual void TouchesBegan(float deviceX, float deviceY);

    /*
    * @brief 拖拽移动事件（单点）
    *
    * @param[in] deviceX 屏幕上触摸的 x 值
    * @param[in] deviceY 屏幕上触摸的 y 値
    */
    virtual void TouchesMoved(float deviceX, float deviceY);

    /*
    * @brief 拖拽移动事件（双点）
    *
    * @param[in] deviceX1 第1个触点的 x 值
    * @param[in] deviceY1 第1个触点的 y 值
    * @param[in] deviceX2 第2个触点的 x 值
    * @param[in] deviceY2 第2个触点的 y 値
    */
    virtual void TouchesMoved(float deviceX1, float deviceY1, float deviceX2, float deviceY2);

    /*
    * @brief 计算滑动（flick）距离
    *
    * @return 滑动距离
    */
    virtual float GetFlickDistance() const;

protected:
    /*
    * @brief 计算两点间距离
    *
    * @param[in] x1 第1点的 x 值
    * @param[in] y1 第1点的 y 值
    * @param[in] x2 第2点的 x 值
    * @param[in] y2 第2点的 y 値
    * @return 两点间距离
    */
    virtual float CalculateDistance(float x1, float y1, float x2, float y2) const;

    /*
    * 根据两个移动量计算最终的移动量。
    * 若方向相反则返回 0；若方向相同则返回绝对值较小者（带符号）。
    *
    * @param[in] v1 第1个移动量
    * @param[in] v2 第2个移动量
    * @return 返回较小的移动量（含方向）
    */
    virtual float CalculateMovingAmount(float v1, float v2);

    float _startY;              // 开始触摸时的 Y 值（原注释为 x，但保持原文含义）
    float _startX;              // 开始触摸时的 X 值（原注释为 y，但保持原文含义）
    float _lastX;               // 单点触摸时的 X 值
    float _lastY;               // 单点触摸时的 Y 值
    float _lastX1;              // 双点触摸时第1个点的 X 值
    float _lastY1;              // 双点触摸时第1个点的 Y 值
    float _lastX2;              // 双点触摸时第2个点的 X 值
    float _lastY2;              // 双点触摸时第2个点的 Y 值
    float _lastTouchDistance;   // 当有两根或以上手指触摸时的距离
    float _deltaX;              // 与上一次相比 X 方向的移动距离
    float _deltaY;              // 与上一次相比 Y 方向的移动距离
    float _scale;               // 本帧用于缩放的倍率，非缩放操作时为 1
    bool _touchSingle;          // 是否为单点触摸
    bool _flipAvailable;        // 是否允许滑动（flick）
};
