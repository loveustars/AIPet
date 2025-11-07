/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppWavFileHandler_Common.hpp"
#include <cmath>
#include <cstdint>
#include "LAppPal.hpp"

LAppWavFileHandler_Common::LAppWavFileHandler_Common()
    : _rawData(NULL)
    , _rawDataSize(0)
    , _pcmData(NULL)
    , _sampleOffset(0)
    , _lastRms(0.0f)
    , _userTimeSeconds(0.0f)
{
}

LAppWavFileHandler_Common::~LAppWavFileHandler_Common()
{
    if (_rawData != NULL)
    {
        CSM_FREE(_rawData);
    }

    if (_pcmData != NULL)
    {
        ReleasePcmData();
    }
}

Csm::csmBool LAppWavFileHandler_Common::Update(Csm::csmFloat32 deltaTimeSeconds)
{
    Csm::csmUint32 goalOffset;
    Csm::csmFloat32 rms;

    // 若尚未加载数据或已到文件末尾则不更新
    if ((_pcmData == NULL)
        || (_sampleOffset >= _wavFileInfo._samplesPerChannel))
    {
        _lastRms = 0.0f;
        return false;
    }

    // 根据经过的时间计算目标样本偏移
    _userTimeSeconds += deltaTimeSeconds;
    goalOffset = static_cast<Csm::csmUint32>(_userTimeSeconds * _wavFileInfo._samplingRate);
    if (goalOffset > _wavFileInfo._samplesPerChannel)
    {
        goalOffset = _wavFileInfo._samplesPerChannel;
    }

    // 计算 RMS（均方根）值
    rms = 0.0f;
    for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++)
    {
        for (Csm::csmUint32 sampleCount = _sampleOffset; sampleCount < goalOffset; sampleCount++)
        {
            Csm::csmFloat32 pcm = _pcmData[channelCount][sampleCount];
            rms += pcm * pcm;
        }
    }
    rms = sqrt(rms / (_wavFileInfo._numberOfChannels * (goalOffset - _sampleOffset)));

    _lastRms = rms;
    _sampleOffset = goalOffset;
    return true;
}

void LAppWavFileHandler_Common::Start(const Csm::csmString& filePath)
{
    // 加载 WAV 文件
    if (!LoadWavFile(filePath))
    {
        return;
    }

    // 初始化样本读取偏移与计时
    _sampleOffset = 0;
    _userTimeSeconds = 0.0f;

    // 重置 RMS 值
    _lastRms = 0.0f;
}

Csm::csmFloat32 LAppWavFileHandler_Common::GetRms() const
{
    return _lastRms;
}

const LAppWavFileHandler_Common::WavFileInfo& LAppWavFileHandler_Common::GetWavFileInfo() const
{
    return _wavFileInfo;
}

const Csm::csmByte* LAppWavFileHandler_Common::GetRawData() const
{
    return _rawData;
}

Csm::csmUint64 LAppWavFileHandler_Common::GetRawDataSize() const
{
    return _rawDataSize;
}

Csm::csmVector<Csm::csmFloat32> LAppWavFileHandler_Common::GetPcmData() const
{
    Csm::csmVector<Csm::csmFloat32> buffer;

    for (Csm::csmUint32 sampleCount = 0; sampleCount < _wavFileInfo._samplesPerChannel; sampleCount++)
    {
        for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++)
        {
            buffer.PushBack(_pcmData[channelCount][sampleCount]);
        }
    }

    return buffer;
}

void LAppWavFileHandler_Common::GetPcmDataChannel(Csm::csmFloat32* dst, Csm::csmUint32 useChannel) const
{
    for (Csm::csmUint32 sampleCount = 0; sampleCount < _wavFileInfo._samplesPerChannel; sampleCount++)
    {
        dst[sampleCount] = _pcmData[useChannel][sampleCount];
    }
}

Csm::csmFloat32 LAppWavFileHandler_Common::NormalizePcmSample(Csm::csmUint32 bitsPerSample, Csm::csmByte* data, Csm::csmUint32 dataSize)
{
    Csm::csmInt32 pcm32;

    // 将样本扩展到 32 位宽，然后归一化到 [-1, 1]
    switch (bitsPerSample)
    {
    case 8:
        if (1 <= dataSize)
        {
            const Csm::csmUint8 ret = data[0];
            pcm32 = static_cast<Csm::csmInt32>(ret) - 128;
            pcm32 <<= 24;
        }
        else
        {
            pcm32 = 0;
        }
        break;
    case 16:
        if (2 <= dataSize)
        {
            const Csm::csmUint16 ret = (data[1] << 8) | data[0];
            pcm32 = ret << 16;
        }
        else
        {
            pcm32 = 0;
        }
        break;
    case 24:
        if (3 <= dataSize)
        {
            const Csm::csmUint32 ret = (data[2] << 16) | (data[1] << 8) | data[0];
            pcm32 = ret << 8;
        }
        else
        {
            pcm32 = 0;
        }
        break;
    case 32:
        if (4 <= dataSize)
        {
            const Csm::csmUint32 ret = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
            pcm32 = ret << 0;
        }
        else
        {
            pcm32 = 0;
        }
        break;
    default:
        // 不支持的位深
        pcm32 = 0;
        break;
    }

    return static_cast<Csm::csmFloat32>(pcm32) / INT32_MAX;
}

Csm::csmBool LAppWavFileHandler_Common::LoadWavFile(const Csm::csmString& filePath)
{
    Csm::csmBool ret;

    // 如果已加载过 wav，则先释放之前的内存
    if (_rawData != NULL)
    {
        CSM_FREE(_rawData);
        _rawDataSize = 0;
    }
    if (_pcmData != NULL)
    {
        ReleasePcmData();
    }

    // 读取文件到内存
    _byteReader._fileByte = LAppPal::LoadFileAsBytes(filePath.GetRawString(), &(_byteReader._fileSize));
    _byteReader._readOffset = 0;

    // 读取失败或文件小于 4 字节（无法包含 "RIFF"），则失败
    if ((_byteReader._fileByte == NULL) || (_byteReader._fileSize < 4))
    {
        return false;
    }

    // 保存文件名
    _wavFileInfo._fileName = filePath;

    do {
        // 检查签名 "RIFF"
        if (!_byteReader.GetCheckSignature("RIFF"))
        {
            ret = false;
            break;
        }
        // 跳过文件大小 - 8
        _byteReader.Get32LittleEndian();
        // 检查签名 "WAVE"
        if (!_byteReader.GetCheckSignature("WAVE"))
        {
            ret = false;
            break;
        }
        // 检查签名 "fmt "
        if (!_byteReader.GetCheckSignature("fmt "))
        {
            ret = false;
            break;
        }
        // 读取 fmt 块大小
        const Csm::csmUint32 fmtChunkSize = _byteReader.Get32LittleEndian();
        // 仅接受格式 ID 为 1（线性 PCM）的文件
        if (_byteReader.Get16LittleEndian() != 1)
        {
            ret = false;
            break;
        }
        // 通道数
        _wavFileInfo._numberOfChannels = _byteReader.Get16LittleEndian();
        // 采样率
        _wavFileInfo._samplingRate = _byteReader.Get32LittleEndian();
        // 平均字节速率
        _wavFileInfo._avgBytesPerSec = _byteReader.Get32LittleEndian();
        // 块对齐大小
        _wavFileInfo._blockAlign = _byteReader.Get16LittleEndian();
        // 每样本位数
        _wavFileInfo._bitsPerSample = _byteReader.Get16LittleEndian();
        // 如果 fmt 块有扩展字段则跳过
        if (fmtChunkSize > 16)
        {
            _byteReader._readOffset += (fmtChunkSize - 16);
        }
        // 跳过直到遇到 "data" 块
        while (!(_byteReader.GetCheckSignature("data"))
            && (_byteReader._readOffset < _byteReader._fileSize))
        {
            _byteReader._readOffset += _byteReader.Get32LittleEndian();
        }
        // 未找到 "data" 块
        if (_byteReader._readOffset >= _byteReader._fileSize)
        {
            ret = false;
            break;
        }
        // 读取样本数
        {
            const Csm::csmUint32 dataChunkSize = _byteReader.Get32LittleEndian();
            _wavFileInfo._samplesPerChannel = (dataChunkSize * 8) / (_wavFileInfo._bitsPerSample * _wavFileInfo._numberOfChannels);
        }
        // 分配内存
        _rawDataSize = static_cast<Csm::csmUint64>(_wavFileInfo._blockAlign) * static_cast<Csm::csmUint64>(_wavFileInfo._samplesPerChannel);
        _rawData = static_cast<Csm::csmByte*>(CSM_MALLOC(sizeof(Csm::csmByte) * _rawDataSize));
        _pcmData = static_cast<Csm::csmFloat32**>(CSM_MALLOC(sizeof(Csm::csmFloat32*) * _wavFileInfo._numberOfChannels));
        for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++)
        {
            _pcmData[channelCount] = static_cast<Csm::csmFloat32*>(CSM_MALLOC(sizeof(Csm::csmFloat32) * _wavFileInfo._samplesPerChannel));
        }
        // 读取波形数据
        Csm::csmUint64 rawPos = 0;
        for (Csm::csmUint32 sampleCount = 0; sampleCount < _wavFileInfo._samplesPerChannel; sampleCount++)
        {
            for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++)
            {
                // 读取未归一化的原始字节
                for (Csm::csmUint32 byteCount = 0; byteCount < _wavFileInfo._bitsPerSample / 8; byteCount++)
                {
                    _rawData[rawPos++] = _byteReader._fileByte[_byteReader._readOffset + byteCount];
                }
                // 归一化到 -1～1 的浮点数
                _pcmData[channelCount][sampleCount] = GetPcmSample();
            }
        }

        ret = true;

    }  while (false);

    // 释放读取的文件字节数据
    LAppPal::ReleaseBytes(_byteReader._fileByte);
    _byteReader._fileByte = NULL;
    _byteReader._fileSize = 0;

    return ret;
}

void LAppWavFileHandler_Common::ReleasePcmData()
{
    for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++)
    {
        CSM_FREE(_pcmData[channelCount]);
    }
    CSM_FREE(_pcmData);
    _pcmData = NULL;
}

Csm::csmFloat32 LAppWavFileHandler_Common::GetPcmSample()
{
    Csm::csmInt32 pcm32;

    // 将样本扩展到 32 位宽并归一化到 [-1, 1]
    switch (_wavFileInfo._bitsPerSample)
    {
    case 8:
        pcm32 = static_cast<Csm::csmInt32>(_byteReader.Get8()) - 128;
        pcm32 <<= 24;
        break;
    case 16:
        pcm32 = _byteReader.Get16LittleEndian() << 16;
        break;
    case 24:
        pcm32 = _byteReader.Get24LittleEndian() << 8;
        break;
    default:
        // 不支持的位深
        pcm32 = 0;
        break;
    }

    return static_cast<Csm::csmFloat32>(pcm32) / INT32_MAX;
}
