cmake_minimum_required(VERSION 3.16)

# 设置应用名称
set(APP_NAME AIPet)

# 设置目录路径
set(SDK_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/CubismSdkForNative)
set(CORE_PATH ${SDK_ROOT_PATH}/Core)
set(FRAMEWORK_PATH ${SDK_ROOT_PATH}/Framework)
set(THIRD_PARTY_PATH ${SDK_ROOT_PATH}/Samples/OpenGL/thirdParty)
set(STB_PATH ${THIRD_PARTY_PATH}/stb)
set(GLEW_PATH ${THIRD_PARTY_PATH}/glew)
set(GLFW_PATH ${THIRD_PARTY_PATH}/glfw)
set(COMMON_SRC_PATH ${SDK_ROOT_PATH}/Samples/Common)

# 设置项目
project(${APP_NAME})

# 定义输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# 设置配置 (Release 和 Debug)
set(CMAKE_CONFIGURATION_TYPES Debug Release
  CACHE STRING "Configurations" FORCE
)

# 禁止生成 ZERO_CHECK 项目
set(CMAKE_SUPPRESS_REGENERATION ON)

# 禁止 GLEW 和 GLFW 的额外处理
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_UTILS OFF CACHE BOOL "" FORCE)

# 指定编译器版本
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(GLEW_STATIC)

# 添加 Cubism Core（静态库）
add_library(Live2DCubismCore STATIC IMPORTED)
set_target_properties(Live2DCubismCore
  PROPERTIES
    IMPORTED_LOCATION ${CORE_PATH}/lib/linux/x86_64/libLive2DCubismCore.a
    INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
)

# 添加 GLEW 和 GLFW
add_subdirectory(${GLEW_PATH}/build/cmake ${CMAKE_CURRENT_BINARY_DIR}/glew)
add_subdirectory(${GLFW_PATH} ${CMAKE_CURRENT_BINARY_DIR}/glfw)

# 指定 Cubism Framework 渲染方式
set(FRAMEWORK_SOURCE OpenGL)

# 添加 Cubism Native Framework
add_subdirectory(${FRAMEWORK_PATH} ${CMAKE_CURRENT_BINARY_DIR}/Framework)

# 为 Framework 添加渲染定义
target_compile_definitions(Framework PUBLIC CSM_TARGET_LINUX_GL)

# 为 Framework 添加 GLEW 头文件路径
target_include_directories(Framework PUBLIC ${GLEW_PATH}/include)

# 链接库到 Framework
target_link_libraries(Framework Live2DCubismCore glew_s)

# 查找系统库
find_package(OpenGL REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# ===== Dear ImGui 配置 =====
set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui")
file(GLOB IMGUI_SOURCES
    "${IMGUI_PATH}/*.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp"
)
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC 
    ${IMGUI_PATH}
    ${IMGUI_PATH}/backends
    ${GLFW_PATH}/include      # 添加 GLFW 头文件路径
    ${GLEW_PATH}/include      # 添加 GLEW 头文件路径
)
# ImGui 需要链接 OpenGL
target_link_libraries(imgui_lib PUBLIC ${OPENGL_LIBRARIES})

# ===== 创建可执行文件 =====
add_executable(${APP_NAME}
    # 主程序
    main.cpp
    
    # AI 管理模块
    src/AIManager.cpp
    src/AIManager.hpp
    
    # Live2D Common 基类
    src/LAppAllocator_Common.cpp
    src/LAppAllocator_Common.hpp
    src/LAppModel_Common.cpp
    src/LAppModel_Common.hpp
    src/MouseActionManager_Common.cpp
    src/MouseActionManager_Common.hpp
    src/LAppTextureManager_Common.cpp
    src/LAppTextureManager_Common.hpp
    src/TouchManager_Common.cpp
    src/TouchManager_Common.hpp
    src/CubismSampleViewMatrix_Common.cpp
    src/CubismSampleViewMatrix_Common.hpp

    
    # Live2D 官方示例代码（简化版）
    src/LAppDefine.cpp
    src/LAppDefine.hpp
    src/LAppPal.cpp
    src/LAppPal.hpp
    src/LAppTextureManager.cpp
    src/LAppTextureManager.hpp
    src/CubismUserModelExtend.cpp
    src/CubismUserModelExtend.hpp
    src/MouseActionManager.cpp
    src/MouseActionManager.hpp
)

# ===== 链接所有库 =====
target_link_libraries(${APP_NAME}
    Framework
    glfw
    imgui_lib
    ${OPENGL_LIBRARIES}
    ${CURL_LIBRARIES}
    Threads::Threads
)

# ===== 指定头文件搜索路径 =====
target_include_directories(${APP_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/nlohmann
    ${STB_PATH}
    ${CURL_INCLUDE_DIR}
)

# ===== 复制资源文件到构建目录 =====
add_custom_command(
  TARGET ${APP_NAME}
  POST_BUILD
  # 复制 Live2D 模型资源
  COMMAND ${CMAKE_COMMAND} -E copy_directory 
    ${CMAKE_CURRENT_SOURCE_DIR}/assets 
    $<TARGET_FILE_DIR:${APP_NAME}>/assets
  # 复制着色器文件
  COMMAND ${CMAKE_COMMAND} -E copy_directory 
    ${FRAMEWORK_PATH}/src/Rendering/OpenGL/Shaders 
    $<TARGET_FILE_DIR:${APP_NAME}>/FrameworkShaders
)

message(STATUS "=== AIPet Configuration ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Cubism SDK Path: ${SDK_ROOT_PATH}")
message(STATUS "==========================")