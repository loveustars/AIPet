/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <ICubismModelSetting.hpp>

/**
 * @brief 用户实际使用的模型实现类
 *
 * 负责模型的生成、功能组件的创建、更新处理以及渲染调用。
 *
 */
class LAppModel_Common : public Csm::CubismUserModel
{
public:
    /**
     * @brief 构造函数
     */
    LAppModel_Common() : Csm::CubismUserModel() {};

    /**
     * @brief 析构函数
     */
    virtual ~LAppModel_Common() {};

protected:
    virtual Csm::csmByte* CreateBuffer(const Csm::csmChar* path, Csm::csmSizeInt* size);
    virtual void DeleteBuffer(Csm::csmByte* buffer, const Csm::csmChar* path = "");
};
