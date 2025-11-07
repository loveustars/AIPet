/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <string>

#include <Type/CubismBasicType.hpp>
#include <Type/csmVector.hpp>

/**
 * @brief 纹理管理类
 *
 * 负责图片的加载与管理。
 */
class LAppTextureManager_Common
{
public:
    /**
     * @brief 图片信息结构体
     */
    struct TextureInfo
    {
        Csm::csmUint32 id;      ///< 纹理 ID
        int width;              ///< 宽度
        int height;             ///< 高度
        std::string fileName;   ///< 文件名
    };

    /**
     * @brief 构造函数
     */
    LAppTextureManager_Common();

    /**
     * @brief 析构函数
     */
    virtual ~LAppTextureManager_Common();

    /**
     * @brief 释放 texturesInfo 中的资源
     */
    virtual void ReleaseTexturesInfo();

    /**
     * @brief 预乘（Premultiply）处理
     *
     * @param[in] red   红色分量
     * @param[in] green 绿色分量
     * @param[in] blue  蓝色分量
     * @param[in] alpha 透明度分量
     * @return  预乘后得到的颜色值
     */
    virtual inline unsigned int Premultiply(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
    {
        return static_cast<unsigned>(\
            (red * (alpha + 1) >> 8) | \
            ((green * (alpha + 1) >> 8) << 8) | \
            ((blue * (alpha + 1) >> 8) << 16) | \
            (((alpha)) << 24)   \
            );
    }

    /**
     * @brief 根据文件名获取纹理信息
     *
     * @param[in] fileName 纹理的文件名
     * @return 如果存在则返回对应的 TextureInfo
     */
    virtual TextureInfo* GetTextureInfoByName(std::string& fileName) const;

    /**
     * @brief 根据纹理 ID 获取纹理信息
     *
     * @param[in] textureId 要查询的纹理 ID
     * @return 如果存在则返回对应的 TextureInfo
     */
    virtual TextureInfo* GetTextureInfoById(Csm::csmUint32 textureId) const;

protected:
    Csm::csmVector<TextureInfo*> _texturesInfo;         ///< 纹理信息列表
};
