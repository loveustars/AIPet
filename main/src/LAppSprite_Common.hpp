/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>
#include <Type/CubismBasicType.hpp>

 /**
 * @brief 实现精灵（Sprite）的类。
 *
 * 管理纹理 ID 和 Rect 信息。
 *
 */
class LAppSprite_Common
{
public:
    /**
     * @brief Rect 结构体
     */
    struct Rect
    {
    public:
        float left;     ///< 左辺
        float right;    ///< 右辺
        float up;       ///< 上辺
        float down;     ///< 下辺
    };

    /**
     * @brief 默认构造函数
     */
    LAppSprite_Common();

    /**
     * @brief 构造函数
     *
     * @param[in] textureId 纹理 ID
     */
    LAppSprite_Common(Csm::csmUint64 textureId);

    /**
     * @brief 析构函数
     */
    virtual ~LAppSprite_Common();

    /**
     * @brief 获取纹理 ID
     * @return 返回纹理 ID
     */
    virtual Csm::csmUint64 GetTextureId() { return _textureId; }

protected:
    Csm::csmUint64 _textureId;  ///< 纹理 ID
};
