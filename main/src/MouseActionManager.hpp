#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <CubismFramework.hpp>

#include "MouseActionManager_Common.hpp"

/**
 * @brief 通知鼠标操作的类（用于 OpenGL 平台）
 *
 * 将鼠标事件桥接到 Cubism。
 */
class MouseActionManager : public MouseActionManager_Common
{
public:
    /**
     * @brief 返回单例实例
     *
     * 若实例尚未创建则内部创建并返回。
     * @return 单例实例指针
     */
  static MouseActionManager* GetInstance();

    /**
     * @brief 释放单例实例
     */
  static void ReleaseInstance();

  MouseActionManager(); ///< 构造函数
  virtual ~MouseActionManager(); ///< 析构函数

    /**
     * @brief GLFW 鼠标按键回调适配函数（用于 OpenGL）
     *
     * @param[in] window 回调所属的窗口
     * @param[in] button 鼠标按键类型
     * @param[in] action 按键动作（按下/释放）
     * @param[in] modify 修饰键标志
     */
  void OnMouseCallBack(GLFWwindow* window, int button, int action, int modify);

    /**
     * @brief GLFW 光标位置回调适配函数（用于 OpenGL）
     *
     * @param[in] window 回调所属的窗口
     * @param[in] x 光标 X 坐标
     * @param[in] y 光标 Y 坐标
     */
  void OnMouseCallBack(GLFWwindow* window, double x, double y);
  
  /**
   * @brief 鼠标滚轮回调
   */
  void OnScroll(GLFWwindow* window, double xoffset, double yoffset);

protected:
  // 当中键按下时用于平移视图的标记
  bool _middleCaptured;
  // 记录上一次鼠标位置（用于计算中键平移的增量）
  float _lastMouseX;
  float _lastMouseY;
};

class EventHandler
{
public:
  /**
  * @brief   用于 glfwSetMouseButtonCallback 的回调函数。
  */
    static void OnMouseCallBack(GLFWwindow* window, int button, int action, int modify)
    {
        MouseActionManager::GetInstance()->OnMouseCallBack(window, button, action, modify);
    }

  /**
  * @brief   用于 glfwSetCursorPosCallback 的回调函数。
  */
    static void OnMouseCallBack(GLFWwindow* window, double x, double y)
    {
        MouseActionManager::GetInstance()->OnMouseCallBack(window, x, y);
    }
  static void OnScroll(GLFWwindow* window, double xoffset, double yoffset)
  {
    MouseActionManager::GetInstance()->OnScroll(window, xoffset, yoffset);
  }
};
