/**
 * @file LAppTextureManager.hpp
 */
#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Type/csmVector.hpp>

#include "LAppTextureManager_Common.hpp"

/**
 * @brief 纹理管理类（继承自 LAppTextureManager_Common）
 *
 * 负责图像的加载与管理。
 */
class LAppTextureManager : public LAppTextureManager_Common
{
public:
    /**
    * @brief 构造函数
    */
    LAppTextureManager();

    /**
    * @brief 析构函数
    */
    ~LAppTextureManager();

    /**
    * @brief 从 PNG 文件创建纹理
    *
    * @param[in] fileName 要加载的图片文件路径
    * @return 纹理信息，加载失败时返回 NULL
    */
    TextureInfo* CreateTextureFromPngFile(std::string fileName);

    /**
    * @brief 释放所有纹理
    *
    * 释放存储在数组中的所有纹理
    */
    void ReleaseTextures();

    /**
     * @brief 释放指定纹理 ID 的纹理
     *
     * @param[in] textureId 要释放的纹理 ID
     */
    void ReleaseTexture(Csm::csmUint32 textureId);

    /**
    * @brief 释放指定文件名对应的纹理
    *
    * @param[in] fileName 要释放的纹理文件路径
    */
    void ReleaseTexture(std::string fileName);
};
