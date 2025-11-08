/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <functional>
#include <string>

#include <CubismFramework.hpp>
#include <CubismModelSettingJson.hpp>

#include "LAppTextureManager.hpp"
#include "LAppModel_Common.hpp"

 /**
 * @brief 继承自 CubismUserModel 的示例类
 *
 * 这是一个继承自 CubismUserModel 的示例类。
 * 可以在此类的基础上扩展并实现自定义处理。
 * 添加了一些辅助方法用于表情切换。
 *
 */
class CubismUserModelExtend :
    public LAppModel_Common
{
public:
    CubismUserModelExtend(const std::string modelDirectoryName, const std::string _currentModelDirectory); ///< 构造函数
    ~CubismUserModelExtend(); ///< 析构函数

    /**
    * @brief 从包含 model3.json 的目录和文件路径生成模型
    */
    void LoadAssets(const  Csm::csmChar* fileName);

    /**
    * @brief 更新模型
    *
    * 更新模型的状态和渲染
    */
    void ModelOnUpdate(GLFWwindow* window);

    /**
    * @brief 根据 AI 回复文本选择并切换表情（简单关键词映射）
    * @param[in] text AI 回复文本
    */
    void SetExpressionByAIText(const std::string& text);

    /**
    * @brief 按表情名切换表情（表情名与 model3.json 中的 Name 字段对应）
    * @param[in] name 表情名字（例如 "F01"）
    */
    void SetExpressionByName(const std::string& name);

    /**
    * @brief 返回已加载的表情名称列表（用于调试/UI）
    */
    std::vector<std::string> GetExpressionNames() const;

    /**
    * @brief 设置默认表情持续时间（秒），<=0 表示不自动恢复
    */
    void SetDefaultExpressionDuration(float seconds) { _defaultExpressionDuration = seconds; }

private:
    /**
    * @brief 从 model3.json 生成模型
    *
    * 根据 model3.json 的描述生成模型，并创建动作、物理等组件
    *
    * @param[in]   setting     ICubismModelSetting 的实例
    */
    void SetupModel();
    /**
    * @brief 开始播放由参数指定的动作
    *
    * @param[in] group    动作组名
    * @param[in] no       组内序号
    * @param[in] priority 优先级
    * @return 返回已开始动作的识别编号，用于 IsFinished() 判断。无法开始时返回 -1。
    */
    Csm::CubismMotionQueueEntryHandle StartMotion(const Csm::csmChar* group, Csm::csmInt32 no, Csm::csmInt32 priority);

    /**
    * @brief 释放资源
    *
    * 释放模型设置相关的数据
    */
    void ReleaseModelSetting();

    /**
    * @brief 绘制模型。传入用于绘制模型的视图-投影矩阵
    *
    * @param[in]  matrix  视图-投影矩阵
    */
    void Draw(Csm::CubismMatrix44& matrix);

    /**
    * @brief 更新模型参数信息
    *
    * 更新模型的参数信息
    */
    void ModelParamUpdate();


    /**
    * @brief 将纹理加载到 OpenGL 的纹理单元中
    */
    void SetupTextures();

    // 内部播放表情（带持续时间），durationSeconds<=0 表示不自动恢复
    void PlayExpression(const std::string& name, float durationSeconds);

    /**
    * @brief 根据组名批量加载动作数据
    *
    * 动作数据的名称从 ModelSetting 中获取
    *
    * @param[in]   group  动作组名
    */
    void PreloadMotionGroup(const Csm::csmChar * group);

    std::string _modelDirName; ///< 存放模型设置的目录名称
    std::string _currentModelDirectory; ///< 当前模型目录

    Csm::csmFloat32 _userTimeSeconds; ///< 累计的时间差[秒]
    Csm::CubismModelSettingJson* _modelJson; ///< 模型设置信息
    Csm::csmVector<Csm::CubismIdHandle> _eyeBlinkIds; ///< 模型设置的眨眼参数 ID
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*> _motions; ///< 已加载的动作列表
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*> _expressions; ///< 已加载的表情列表

    LAppTextureManager* _textureManager;         ///< 纹理管理器

    const Csm::CubismId* _idParamAngleX; ///< 参数 ID: ParamAngleX
    const Csm::CubismId* _idParamAngleY; ///< 参数 ID: ParamAngleY
    const Csm::CubismId* _idParamAngleZ; ///< 参数 ID: ParamAngleZ
    const Csm::CubismId* _idParamBodyAngleX; ///< 参数 ID: ParamBodyAngleX
    const Csm::CubismId* _idParamEyeBallX; ///< 参数 ID: ParamEyeBallX
    const Csm::CubismId* _idParamEyeBallY; ///< 参数 ID: ParamEyeBallY

    // 当前表情控制（用于超时恢复）
    std::string _currentExpressionName; ///< 当前播放的表情名
    float _defaultExpressionDuration = 5.0f; ///< 表情自动恢复到中性所用的默认持续时间（秒）
    float _expressionDuration = 0.0f; ///< 当前表情的持续时间（若>0表示会在到期后恢复）
    double _expressionSetTime = 0.0; ///< 设置当前表情时的累计时间点（以 _userTimeSeconds 计）
    bool _expressionTemporary = false; ///< 当前表情是否为临时表情（到期后恢复）
};
