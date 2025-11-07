/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>

/**
 * @brief Sample App 使用的常量
 *
 */
namespace LAppDefine {

    using namespace Csm;

    extern const csmFloat32 ViewScale;              ///< 缩放比例
    extern const csmFloat32 ViewMaxScale;           ///< 最大缩放比例
    extern const csmFloat32 ViewMinScale;           ///< 最小缩放比例

    extern const csmFloat32 ViewLogicalLeft;        ///< 逻辑视图坐标系左边界值
    extern const csmFloat32 ViewLogicalRight;       ///< 逻辑视图坐标系右边界值
    extern const csmFloat32 ViewLogicalBottom;      ///< 逻辑视图坐标系下边界值
    extern const csmFloat32 ViewLogicalTop;         ///< 逻辑视图坐标系上边界值

    extern const csmFloat32 ViewLogicalMaxLeft;     ///< 逻辑视图坐标系左边界的最大值
    extern const csmFloat32 ViewLogicalMaxRight;    ///< 逻辑视图坐标系右边界的最大值
    extern const csmFloat32 ViewLogicalMaxBottom;   ///< 逻辑视图坐标系下边界的最大值
    extern const csmFloat32 ViewLogicalMaxTop;      ///< 逻辑视图坐标系上边界的最大值

    extern const csmChar* ResourcesPath;            ///< 素材路径
    extern const csmChar* BackImageName;         ///< 背景图片文件
    extern const csmChar* GearImageName;         ///< 齿轮图片文件
    extern const csmChar* PowerImageName;        ///< 退出按钮图片文件

    extern const csmChar* ShaderPath;               ///< 着色器路径
    extern const csmChar* VertShaderName;           ///< 顶点着色器
    extern const csmChar* FragShaderName;           ///< 片段着色器

    // 模型定义--------------------------------------------
                                                    // 与外部定义文件 (json) 保持一致
    extern const csmChar* MotionGroupIdle;          ///< 空闲时播放的动作组名
    extern const csmChar* MotionGroupTapBody;       ///< 点击身体时播放的动作组名

                                                    // 与外部定义文件 (json) 保持一致
    extern const csmChar* HitAreaNameHead;          ///< 碰撞区域的 [Head] 标签
    extern const csmChar* HitAreaNameBody;          ///< 碰撞区域的 [Body] 标签

                                                    // 动作优先级常量
    extern const csmInt32 PriorityNone;             ///< 动作优先级常量: 0
    extern const csmInt32 PriorityIdle;             ///< 动作优先级常量: 1
    extern const csmInt32 PriorityNormal;           ///< 动作优先级常量: 2
    extern const csmInt32 PriorityForce;            ///< 动作优先级常量: 3

                                                    // 调试日志输出设置
    extern const csmBool DebugLogEnable;            ///< 是否启用调试日志输出
    extern const csmBool DebugTouchLogEnable;       ///< 是否启用触摸处理的调试日志

    // 框架输出的日志级别设置
    extern const CubismFramework::Option::LogLevel CubismLoggingLevel; ///< 框架日志级别

    // 默认的渲染目标尺寸
    extern const csmInt32 RenderTargetWidth;  ///< 默认渲染目标宽度
    extern const csmInt32 RenderTargetHeight; ///< 默认渲染目标高度
}
