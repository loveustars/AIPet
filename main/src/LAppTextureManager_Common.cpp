/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppTextureManager_Common.hpp"

LAppTextureManager_Common::LAppTextureManager_Common()
{
}

LAppTextureManager_Common::~LAppTextureManager_Common()
{
    // 析构时释放所有纹理信息
    ReleaseTexturesInfo();
}

void LAppTextureManager_Common::ReleaseTexturesInfo()
{
    // 释放 _texturesInfo 中的每一项并清空容器
    for (Csm::csmUint32 i = 0; i < _texturesInfo.GetSize(); i++)
    {
        delete _texturesInfo[i];
    }

    _texturesInfo.Clear();
}

LAppTextureManager_Common::TextureInfo* LAppTextureManager_Common::GetTextureInfoByName(std::string& fileName) const
{
    // 遍历已加载的纹理列表，按文件名匹配
    for (Csm::csmUint32 i = 0; i < _texturesInfo.GetSize(); i++)
    {
        if (_texturesInfo[i]->fileName == fileName)
        {
            return _texturesInfo[i];
        }
    }

    return NULL;
}

LAppTextureManager_Common::TextureInfo* LAppTextureManager_Common::GetTextureInfoById(Csm::csmUint32 textureId) const
{
    // 遍历已加载的纹理列表，按 ID 匹配
    for (Csm::csmUint32 i = 0; i < _texturesInfo.GetSize(); i++)
    {
        if (_texturesInfo[i]->id == textureId)
        {
            return _texturesInfo[i];
        }
    }

    return NULL;
}
