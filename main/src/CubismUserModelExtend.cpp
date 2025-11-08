/**
 * Copyright(c) Live2D Inc. All rights reserved.
/**
 * @brief   开始播放由参数指定的动作（motion）。
 *
 * @param[in]   group       动作组名称
 * @param[in]   no          组内的序号
 * @param[in]   priority    优先级
 * @return                  返回已开始动作的识别编号。用于判断单个动作是否结束的 IsFinished() 的参数。如果无法开始则返回 -1。
 */
#include <Utils/CubismString.hpp>
#include <Motion/CubismMotion.hpp>
#include <Physics/CubismPhysics.hpp>
#include <CubismDefaultParameterId.hpp>
#include <Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp>
#include <Motion/CubismMotionQueueEntry.hpp>
#include <Id/CubismIdManager.hpp>

#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "MouseActionManager.hpp"

#include "CubismUserModelExtend.hpp"
#include <iostream>

using namespace Live2D::Cubism::Framework;
using namespace DefaultParameterId;
using namespace LAppDefine;

CubismUserModelExtend::CubismUserModelExtend(const std::string modelDirectoryName, const std::string _currentModelDirectory)
    : LAppModel_Common()
    , _modelJson(NULL)
    , _userTimeSeconds(0.0f)
    , _modelDirName(modelDirectoryName)
    , _currentModelDirectory(_currentModelDirectory)
    , _textureManager(new LAppTextureManager())
{
    // 获取参数 ID
    _idParamAngleX = CubismFramework::GetIdManager()->GetId(ParamAngleX);
    _idParamAngleY = CubismFramework::GetIdManager()->GetId(ParamAngleY);
    _idParamAngleZ = CubismFramework::GetIdManager()->GetId(ParamAngleZ);
    _idParamBodyAngleX = CubismFramework::GetIdManager()->GetId(ParamBodyAngleX);
    _idParamEyeBallX = CubismFramework::GetIdManager()->GetId(ParamEyeBallX);
    _idParamEyeBallY = CubismFramework::GetIdManager()->GetId(ParamEyeBallY);
}

CubismUserModelExtend::~CubismUserModelExtend()
{
    // 释放模型设置数据
    ReleaseModelSetting();

    // 释放纹理管理器
    delete _textureManager;
}

void CubismUserModelExtend::LoadAssets(const Csm::csmChar* fileName)
{
    csmSizeInt size;
    const csmString path = csmString(_currentModelDirectory.c_str()) + fileName;

    csmByte* buffer = CreateBuffer(path.GetRawString(), &size);
    _modelJson = new CubismModelSettingJson(buffer, size);
    DeleteBuffer(buffer, path.GetRawString());

    // 生成模型
    SetupModel();
}

void CubismUserModelExtend::SetupModel()
{
    _updating = true;
    _initialized = false;

    csmByte* buffer;
    csmSizeInt size;

    //Cubism Model
    if (strcmp(_modelJson->GetModelFileName(), ""))
    {
        csmString path = _modelJson->GetModelFileName();
        path = csmString(_currentModelDirectory.c_str()) + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        LoadModel(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    // 读取表情数据
    if (_modelJson->GetExpressionCount() > 0)
    {
        const csmInt32 count = _modelJson->GetExpressionCount();
        for (csmInt32 i = 0; i < count; i++)
        {
            csmString name = _modelJson->GetExpressionName(i);
            csmString path = _modelJson->GetExpressionFileName(i);
            path = csmString(_currentModelDirectory.c_str()) + path;

            buffer = CreateBuffer(path.GetRawString(), &size);
            ACubismMotion* motion = LoadExpression(buffer, size, name.GetRawString());

            if (motion)
            {
                if (_expressions[name])
                {
                    ACubismMotion::Delete(_expressions[name]);
                    _expressions[name] = nullptr;
                }
                _expressions[name] = motion;
            }

            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    // 读取姿势（pose）数据
    if (strcmp(_modelJson->GetPoseFileName(), ""))
    {
        csmString path = _modelJson->GetPoseFileName();
        path = csmString(_currentModelDirectory.c_str()) + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        LoadPose(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    // 读取物理（physics）数据
    if (strcmp(_modelJson->GetPhysicsFileName(), ""))
    {
        csmString path = _modelJson->GetPhysicsFileName();
        path = csmString(_currentModelDirectory.c_str()) + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        LoadPhysics(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    // 读取模型附带的用户数据
    if (strcmp(_modelJson->GetUserDataFile(), ""))
    {
        csmString path = _modelJson->GetUserDataFile();
        path = csmString(_currentModelDirectory.c_str()) + path;
        buffer = CreateBuffer(path.GetRawString(), &size);
        LoadUserData(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    // 布局
    csmMap<csmString, csmFloat32> layout;
    _modelJson->GetLayoutMap(layout);
    // 从布局信息设置位置
    _modelMatrix->SetupFromLayout(layout);

    // 保存参数
    _model->SaveParameters();

    // 读取动作（motion）数据
    for (csmInt32 i = 0; i < _modelJson->GetMotionGroupCount(); i++)
    {
        const csmChar* group = _modelJson->GetMotionGroupName(i);
    // 根据组名一次性预加载该组的动作数据
    PreloadMotionGroup(group);
    }

    _motionManager->StopAllMotions();

    // 创建渲染器
    CreateRenderer();

    // 设置纹理
    SetupTextures();

    _updating = false;
    _initialized = true;
}

void CubismUserModelExtend::PreloadMotionGroup(const csmChar* group)
{
    // 获取组中注册的动作数量
    const csmInt32 count = _modelJson->GetMotionCount(group);

    for (csmInt32 i = 0; i < count; i++)
    {
    // ex) idle_0
    // 获取动作的文件名与路径
        csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, i);
        csmString path = _modelJson->GetMotionFileName(group, i);
        path = csmString(_currentModelDirectory.c_str()) + path;

        csmByte* buffer;
        csmSizeInt size;
        buffer = CreateBuffer(path.GetRawString(), &size);
    // 读取动作数据
        CubismMotion* tmpMotion = static_cast<CubismMotion*>(LoadMotion(buffer, size, name.GetRawString(), NULL, NULL, _modelJson, group, i));

        if (tmpMotion)
        {
            if (_motions[name])
            {
                // 释放实例
                ACubismMotion::Delete(_motions[name]);
            }
            _motions[name] = tmpMotion;
        }

        DeleteBuffer(buffer, path.GetRawString());
    }
}

void CubismUserModelExtend::ReleaseModelSetting()
{
    // 释放动作（motion）
    for (Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>::const_iterator iter = _motions.Begin(); iter != _motions.End(); ++iter)
    {
        Csm::ACubismMotion::Delete(iter->Second);
    }

    _motions.Clear();

    // 释放所有表情数据
    for (Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>::const_iterator iter = _expressions.Begin(); iter != _expressions.End(); ++iter)
    {
        Csm::ACubismMotion::Delete(iter->Second);
    }

    _expressions.Clear();

    delete(_modelJson);
}

/**
 * @brief 根据参数开始播放指定的动作（motion）。
 *
 * @param[in] group 动作组名称
 * @param[in] no    组内索引
 * @param[in] priority 优先级
 * @return 返回已开始动作的识别 ID。用于 IsFinished() 判断单个动作是否结束。无法开始时返回 -1。
 */
Csm::CubismMotionQueueEntryHandle CubismUserModelExtend::StartMotion(const Csm::csmChar* group, Csm::csmInt32 no, Csm::csmInt32 priority)
{
    // 未能获取到动作数量或数量为 0
    if (!(_modelJson->GetMotionCount(group)))
    {
        return Csm::InvalidMotionQueueEntryHandleValue;
    }

    if (priority == LAppDefine::PriorityForce)
    {
    // 设置已预定动作的优先级
        _motionManager->SetReservePriority(priority);
    }
    else if (!_motionManager->ReserveMotion(priority))
    {
        return Csm::InvalidMotionQueueEntryHandleValue;
    }

    // 获取指定的 .motion3.json 文件名
    const Csm::csmString fileName = _modelJson->GetMotionFileName(group, no);

    // ex) idle_0
    // 生成动作数据对象
    csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, no);
    CubismMotion* motion = static_cast<CubismMotion*>(_motions[name.GetRawString()]);
    csmBool autoDelete = false;

    if (!motion)
    {
        csmString path = fileName;
        path = csmString(_currentModelDirectory.c_str()) + path;

        csmByte* buffer;
        csmSizeInt size;
        buffer = CreateBuffer(path.GetRawString(), &size);
        // 加载第一个（首个）动作
        motion = static_cast<CubismMotion*>(LoadMotion(buffer, size, NULL, NULL, NULL, _modelJson, group, no));

        if (motion)
        {
            // 在结束时从内存中删除
            autoDelete = true;
        }

        DeleteBuffer(buffer, path.GetRawString());
    }

    // 设置优先级并开始动作
    return  _motionManager->StartMotionPriority(motion, autoDelete, priority);
}

void CubismUserModelExtend::ModelParamUpdate()
{
    // 获取与上一帧的时间差
    const Csm::csmFloat32 deltaTimeSeconds = LAppPal::GetDeltaTime();
    _userTimeSeconds += deltaTimeSeconds;

    // 更新拖拽信息
    _dragManager->Update(deltaTimeSeconds);
    _dragX = _dragManager->GetX();
    _dragY = _dragManager->GetY();

    // 是否有动作（motion）更新参数
    Csm::csmBool motionUpdated = false;

    // 加载上一次保存的状态
    _model->LoadParameters();

    if (_motionManager->IsFinished())
    {
    // 如果没有正在播放的动作，则播放初始注册的动作
        StartMotion(LAppDefine::MotionGroupIdle, 0, LAppDefine::PriorityIdle);
    }
    else
    {
    // 更新动作并应用参数
        motionUpdated = _motionManager->UpdateMotion(_model, deltaTimeSeconds);
    }

    // 保存状态
    _model->SaveParameters();

    if (_expressionManager)
    {
        // 通过表情更新参数（相对变化）
        _expressionManager->UpdateMotion(_model, deltaTimeSeconds);
    }

    // 如果当前表情是临时的并且已经过期，则恢复到中性表情(F01)
    if (_expressionTemporary && _expressionDuration > 0.0f)
    {
        if ((_userTimeSeconds - _expressionSetTime) >= _expressionDuration)
        {
            // 恢复到中性并取消临时标记（此调用不应再次设置计时）
            PlayExpression(std::string("F01"), 0.0f);
            _expressionTemporary = false;
            _expressionDuration = 0.0f;
        }
    }

    // 由拖拽引起的变化
    /**
    * 通过拖拽调整面部朝向
    * 添加 -30 到 30 的值
    */
    _model->AddParameterValue(_idParamAngleX, _dragX * 30.0f);
    _model->AddParameterValue(_idParamAngleY, _dragY * 30.0f);
    _model->AddParameterValue(_idParamAngleZ, _dragX * _dragY * -30.0f);

    // 通过拖拽调整身体的朝向
    _model->AddParameterValue(_idParamBodyAngleX, _dragX * 10.0f); // 添加 -10 到 10 的值

    // 通过拖拽调整眼睛朝向
    _model->AddParameterValue(_idParamEyeBallX, _dragX); // 添加 -1 到 1 的值
    _model->AddParameterValue(_idParamEyeBallY, _dragY);

    // 应用物理演算设置（如果存在）
    if (_physics)
    {
        _physics->Evaluate(_model, deltaTimeSeconds);
    }

    // 应用姿势（pose）设置（如果存在）
    if (_pose)
    {
        _pose->UpdateParameters(_model, deltaTimeSeconds);
    }

    // 更新模型参数信息
    _model->Update();
}

void CubismUserModelExtend::Draw(Csm::CubismMatrix44& matrix)
{
    if (!_model)
    {
        return;
    }

    // 将模型矩阵乘到当前矩阵上
    matrix.MultiplyByMatrix(_modelMatrix);

    // 将该矩阵设置为渲染器的 Model-View-Projection 矩阵
    GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2>()->SetMvpMatrix(&matrix);

    // 发出并执行模型绘制命令
    GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2>()->DrawModel();
}

void CubismUserModelExtend::SetupTextures()
{
    for (csmInt32 modelTextureNumber = 0; modelTextureNumber < _modelJson->GetTextureCount(); modelTextureNumber++)
    {
        // 若纹理名为空则跳过加载与绑定
        if (!strcmp(_modelJson->GetTextureFileName(modelTextureNumber), ""))
        {
            continue;
        }

        // 将纹理加载到 OpenGL 的纹理单元
        csmString texturePath = _modelJson->GetTextureFileName(modelTextureNumber);
        texturePath = csmString(_currentModelDirectory.c_str()) + texturePath;

        LAppTextureManager::TextureInfo* texture = _textureManager->CreateTextureFromPngFile(texturePath.GetRawString());
        const csmInt32 glTextueNumber = texture->id;

        // OpenGL
        GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->BindTexture(modelTextureNumber, glTextueNumber);
    }

    // 设置是否启用预乘（premultiplied）alpha
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(false);
}

// 按表情名查找并切换表情（如果存在）
void CubismUserModelExtend::SetExpressionByName(const std::string& name)
{
    // 默认播放并将其设置为临时表情（使用默认持续时间）
    PlayExpression(name, _defaultExpressionDuration);
}

// 根据 AI 文本进行简单关键词映射到表情名，然后调用 SetExpressionByName
void CubismUserModelExtend::SetExpressionByAIText(const std::string& text)
{
    if (text.empty()) return;
    // 中英混合关键词映射
    std::string name = "";
    std::string lower = text;
    for (auto &c : lower) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    auto contains = [&](const std::string &s) {
        return text.find(s) != std::string::npos || lower.find(s) != std::string::npos;
    };

    if (contains("高兴") || contains("开心") || contains("笑") || contains("happy") || contains("glad") || contains("joy"))
    {
        name = "F05"; // 开心
    }
    else if (contains("生气") || contains("愤怒") || contains("气") || contains("angry") || contains("mad"))
    {
        name = "F03"; // 生气
    }
    else if (contains("难过") || contains("伤心") || contains("哭") || contains("sad") || contains("unhappy") || contains("sorrow"))
    {
        name = "F04"; // 悲伤
    }
    else if (contains("惊讶") || contains("惊") || contains("surprise") || contains("surprised") || contains("wow"))
    {
        name = "F06"; // 惊讶
    }
    else if (contains("害羞") || contains("脸红") || contains("不好意思") || contains("shy") || contains("embarrass"))
    {
        name = "F07"; // 害羞
    }
    else if (contains("困惑") || contains("疑惑") || contains("疑问") || contains("confuse") || contains("confused") || contains("huh"))
    {
        name = "F08"; // 疑惑不解
    }
    else if (contains("open") || contains("mouth") || contains("说话") || contains("嘴"))
    {
        name = "F02"; // 张开嘴巴
    }
    else
    {
        // 错误信息映射为悲伤
        if (contains("error:") || contains("failed") || contains("http request failed") || contains("curl_easy_perform") || contains("curl"))
        {
            name = "F04";
        }
        else
        {
            name = "F01"; // 中性作为默认
        }
    }

    // 播放表情，默认使用默认持续时间（会在到期后恢复）
    PlayExpression(name, _defaultExpressionDuration);
}

// 内部通用播放函数，可指定持续时间（<=0 表示不自动恢复）
void CubismUserModelExtend::PlayExpression(const std::string& name, float durationSeconds)
{
    std::cout << "[Expression] Request PlayExpression name='" << name << "' duration=" << durationSeconds << std::endl;
    if (!_expressionManager) {
        std::cout << "[Expression] No _expressionManager available" << std::endl;
        return;
    }

    Csm::ACubismMotion* motion = nullptr;
    for (auto iter = _expressions.Begin(); iter != _expressions.End(); ++iter)
    {
        const std::string k = iter->First.GetRawString();
        if (k == name && iter->Second != nullptr)
        {
            motion = iter->Second;
            break;
        }
    }

    if (!motion) {
        std::cout << "[Expression] Motion '" << name << "' not found in _expressions" << std::endl;
        return;
    }

    std::cout << "[Expression] Found motion for '" << name << "', starting" << std::endl;
    _expressionManager->StartMotion(motion, false);

    _currentExpressionName = name;
    if (durationSeconds > 0.0f)
    {
        _expressionTemporary = true;
        _expressionDuration = durationSeconds;
        _expressionSetTime = _userTimeSeconds;
    }
    else
    {
        _expressionTemporary = false;
        _expressionDuration = 0.0f;
    }
}

std::vector<std::string> CubismUserModelExtend::GetExpressionNames() const
{
    std::vector<std::string> list;
    for (auto iter = _expressions.Begin(); iter != _expressions.End(); ++iter)
    {
        list.emplace_back(iter->First.GetRawString());
    }
    return list;
}

void CubismUserModelExtend::ModelOnUpdate(GLFWwindow* window)
{
    int width, height;
    // 获取窗口尺寸
    glfwGetWindowSize(window, &width, &height);

    Csm::CubismMatrix44 projection;
    // 为安全起见，先重置为单位矩阵
    projection.LoadIdentity();

    if (_model->GetCanvasWidth() > 1.0f && width < height)
    {
        // 当横向更宽的模型在纵向窗口中显示时，根据模型宽度计算 scale
        GetModelMatrix()->SetWidth(2.0f);
        projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height));
    }
    else
    {
        projection.Scale(static_cast<float>(height) / static_cast<float>(width), 1.0f);
    }

    // 如有需要，在此乘上视图矩阵
    if (MouseActionManager::GetInstance()->GetViewMatrix() != NULL)
    {
        projection.MultiplyByMatrix(MouseActionManager::GetInstance()->GetViewMatrix());
    }

    // 更新模型参数
    ModelParamUpdate();

    // 更新模型绘制（projection 作为引用会被修改）
    Draw(projection);
}
