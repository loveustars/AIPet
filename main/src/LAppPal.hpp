/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 * 官方源代码注释是日语的，翻译成中文比较好一点
 */

#pragma once

#include <CubismFramework.hpp>
#include <cstdlib>
#include <string>

/**
 * @brief 抽象化平台相关功能的 Cubism 平台抽象层（Platform Abstraction Layer）。
 *
 * 将文件读取、时间获取等平台相关函数集中封装。
 *
 */
class LAppPal
{
public:
    /**
     * @brief 将文件读取为字节数据
     *
     * @param[in]  filePath  要读取的文件路径
     * @param[out] outSize   输出文件大小
     * @return               读取到的字节数据指针
     */
    static Csm::csmByte* LoadFileAsBytes(const std::string filePath, Csm::csmSizeInt* outSize);


    /**
     * @brief 释放字节数据
     *
     * @param[in] byteData 要释放的字节数据指针
     */
    static void ReleaseBytes(Csm::csmByte* byteData);

    /**
     * @brief 获取与上一帧的时间差（Delta time）
     *
     * @return  返回时间差 [秒]
     */
    static Csm::csmFloat32 GetDeltaTime();

    static void UpdateTime();

    /**
     * @brief 输出日志（不带换行）
     *
     * @param[in] format 格式化字符串
     * @param[in] ...    可变参数
     */
    static void PrintLog(const Csm::csmChar* format, ...);

    /**
     * @brief 输出日志并在末尾换行
     *
     * @param[in] format 格式化字符串
     * @param[in] ...    可变参数
     */
    static void PrintLogLn(const Csm::csmChar* format, ...);

    /**
     * @brief 输出消息（不带换行）
     *
     * @param[in] message 要输出的消息字符串
     */
    static void PrintMessage(const Csm::csmChar* message);

    /**
     * @brief 输出消息并在末尾换行
     *
     * @param[in] message 要输出的消息字符串
     */
    static void PrintMessageLn(const Csm::csmChar* message);

private:
    static double s_currentFrame;
    static double s_lastFrame;
    static double s_deltaTime;
};

