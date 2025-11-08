/**
 * @file LAppDefine.cpp
 * 这个文件实现了 App 使用的常量定义。
 */
#include "LAppDefine.hpp"
#include <CubismFramework.hpp>

namespace LAppDefine {

    using namespace Csm;

    // 视图/屏幕
    const csmFloat32 ViewScale = 1.0f;
    const csmFloat32 ViewMaxScale = 2.0f;
    const csmFloat32 ViewMinScale = 0.8f;

    const csmFloat32 ViewLogicalLeft = -1.0f;
    const csmFloat32 ViewLogicalRight = 1.0f;
    const csmFloat32 ViewLogicalBottom = -1.0f;
    const csmFloat32 ViewLogicalTop = 1.0f;

    const csmFloat32 ViewLogicalMaxLeft = -2.0f;
    const csmFloat32 ViewLogicalMaxRight = 2.0f;
    const csmFloat32 ViewLogicalMaxBottom = -2.0f;
    const csmFloat32 ViewLogicalMaxTop = 2.0f;

    // 相对路径
    const csmChar* ResourcesPath = "Resources/";

    // 模型后方的背景图片文件
    const csmChar* BackImageName = "back_class_normal.png";
    // 歯車
    const csmChar* GearImageName = "icon_gear.png"; // 齿轮图标
    // 退出按钮
    const csmChar* PowerImageName = "close.png";

    // 着色器（shader）相对路径
    const csmChar* ShaderPath = "SampleShaders/";
    // 顶点着色器
    const csmChar* VertShaderName = "VertSprite.vert";
    // 片段（片元）着色器
    const csmChar* FragShaderName = "FragSprite.frag";

    // 模型定义 ------------------------------------------
    // 与外部定义文件（json）对应
    const csmChar* MotionGroupIdle = "Idle"; // 空闲动作
    const csmChar* MotionGroupTapBody = "TapBody"; // 点击身体时的动作

    // 与外部定义文件（json）对应
    const csmChar* HitAreaNameHead = "Head";
    const csmChar* HitAreaNameBody = "Body";

    // 动作（motion）的优先级常量
    const csmInt32 PriorityNone = 0;
    const csmInt32 PriorityIdle = 1;
    const csmInt32 PriorityNormal = 2;
    const csmInt32 PriorityForce = 3;

    // 调试用日志显示选项
    const csmBool DebugLogEnable = true;
    const csmBool DebugTouchLogEnable = false;

    // 框架输出日志等级设置
    const CubismFramework::Option::LogLevel CubismLoggingLevel = CubismFramework::Option::LogLevel_Verbose;

    // 默认的渲染目标尺寸
    const csmInt32 RenderTargetWidth = 1900;
    const csmInt32 RenderTargetHeight = 1000;
}
