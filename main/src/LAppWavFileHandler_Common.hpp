/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>
#include <Type/csmVector.hpp>

/**
 * @brief WAV 文件处理器（只支持 PCM WAV）
 * @note 目前已实现的为 8/16/24/32 位深读取和归一化到浮点 [-1,1]
 */
class LAppWavFileHandler_Common
{
public:
    LAppWavFileHandler_Common();
    virtual ~LAppWavFileHandler_Common();

    // 每帧更新：根据播放时间更新样本偏移并计算 RMS（反映音量）
    virtual Csm::csmBool Update(Csm::csmFloat32 deltaTimeSeconds);

    // 开始加载并准备播放指定路径的 WAV 文件
    virtual void Start(const Csm::csmString& filePath);

    // 获取最近一次计算的 RMS 值
    virtual Csm::csmFloat32 GetRms() const;

    // 获取 WAV 文件信息结构
    virtual const struct WavFileInfo& GetWavFileInfo() const;

    // 获取原始字节数据及其大小（未归一化）
    virtual const Csm::csmByte* GetRawData() const;
    virtual Csm::csmUint64 GetRawDataSize() const;

    // 获取归一化后的 PCM 数据（已交错）
    virtual Csm::csmVector<Csm::csmFloat32> GetPcmData() const;

    // 按通道获取 PCM 数据（非交错）
    virtual void GetPcmDataChannel(Csm::csmFloat32* dst, Csm::csmUint32 useChannel) const;

    // 将指定位深和字节序的数据归一化到浮点范围 [-1,1]
    static Csm::csmFloat32 NormalizePcmSample(Csm::csmUint32 bitsPerSample, Csm::csmByte* data, Csm::csmUint32 dataSize);

protected:
    // 从文件加载 WAV 数据
    virtual Csm::csmBool LoadWavFile(const Csm::csmString& filePath);

    // 释放 PCM 内存
    virtual void ReleasePcmData();

    // 获取下一个样本并归一化（内部使用）
    virtual Csm::csmFloat32 GetPcmSample();

    // 文件字节读取辅助结构
    struct ByteReader {
        ByteReader() : _fileByte(NULL), _fileSize(0), _readOffset(0) { }

        Csm::csmUint8 Get8()
        {
            const Csm::csmUint8 ret = _fileByte[_readOffset];
            _readOffset++;
            return ret;
        }

        Csm::csmUint16 Get16LittleEndian()
        {
            const Csm::csmUint16 ret = (_fileByte[_readOffset + 1] << 8) | _fileByte[_readOffset];
            _readOffset += 2;
            return ret;
        }

        Csm::csmUint32 Get24LittleEndian()
        {
            const Csm::csmUint32 ret =
                (_fileByte[_readOffset + 2] << 16) | (_fileByte[_readOffset + 1] << 8)
                | _fileByte[_readOffset];
            _readOffset += 3;
            return ret;
        }

        Csm::csmUint32 Get32LittleEndian()
        {
            const Csm::csmUint32 ret =
                (_fileByte[_readOffset + 3] << 24) | (_fileByte[_readOffset + 2] << 16)
                | (_fileByte[_readOffset + 1] << 8) | _fileByte[_readOffset];
            _readOffset += 4;
            return ret;
        }

        // 检查接下来的 4 字节是否与给定签名相同（例如 "RIFF", "WAVE", "data"）
        Csm::csmBool GetCheckSignature(const Csm::csmString& reference)
        {
            Csm::csmChar getSignature[4] = { 0, 0, 0, 0 };
            const Csm::csmChar* referenceString = reference.GetRawString();
            if (reference.GetLength() != 4)
            {
                return false;
            }
            for (Csm::csmUint32 signatureOffset = 0; signatureOffset < 4; signatureOffset++)
            {
                getSignature[signatureOffset] = static_cast<Csm::csmChar>(Get8());
            }
            return (getSignature[0] == referenceString[0]) && (getSignature[1] == referenceString[1])
                && (getSignature[2] == referenceString[2]) && (getSignature[3] == referenceString[3]);
        }

        Csm::csmByte* _fileByte; ///< 已加载的文件字节数组
        Csm::csmSizeInt _fileSize; ///< 文件大小
        Csm::csmUint32 _readOffset; ///< 当前读取偏移
    } _byteReader;

    // 成员数据
    struct WavFileInfo {
        Csm::csmString _fileName;
        Csm::csmUint32 _numberOfChannels;
        Csm::csmUint32 _samplingRate;
        Csm::csmUint32 _avgBytesPerSec;
        Csm::csmUint16 _blockAlign;
        Csm::csmUint16 _bitsPerSample;
        Csm::csmUint32 _samplesPerChannel;
    };

    Csm::csmByte* _rawData; ///< 未归一化的原始字节数据
    Csm::csmUint64 _rawDataSize; ///< 原始字节数据大小
    Csm::csmFloat32** _pcmData; ///< 按通道存储的归一化 PCM 数据
    Csm::csmUint32 _sampleOffset; ///< 当前样本偏移
    Csm::csmFloat32 _lastRms; ///< 最近一次计算的 RMS 值
    Csm::csmFloat32 _userTimeSeconds; ///< 累积的播放时间（秒）
};
