/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>

/**
 * @brief 实现内存分配的类。
 *
 * 实现了内存分配与释放的接口。
 * 由框架调用。
 *
 */
class LAppAllocator_Common : public Csm::ICubismAllocator
{
    /**
     * @brief 分配内存区域
     *
     * @param[in] size 要分配的大小
     * @return 分配到的内存指针
     */
    virtual void* Allocate(const Csm::csmSizeType size);

    /**
     * @brief 释放内存区域
     *
     * @param[in] memory 要释放的内存指针
     */
    virtual void Deallocate(void* memory);

    /**
     * @brief 按对齐要求分配内存
     *
     * @param[in] size 要分配的大小
     * @param[in] alignment 对齐字节数
     * @return 对齐后的地址
     */
    virtual void* AllocateAligned(const Csm::csmSizeType size, const Csm::csmUint32 alignment);

    /**
     * @brief 释放对齐分配的内存
     *
     * @param[in] alignedMemory 要释放的对齐内存指针
     */
    virtual void DeallocateAligned(void* alignedMemory);
};
