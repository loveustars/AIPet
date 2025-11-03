cmake_minimum_required(VERSION 3.16)

project(AIPetStep4 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 查找我们需要的系统库 ---
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED) # 我们需要 GLEW
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# --- Cubism SDK 配置 ---
# 添加所有头文件的搜索路径
include_directories(
    src                     # 我们自己的头文件
    lib                     # nlohmann
    lib/Core/include        # Cubism SDK Core
    lib/Framework/include   # Cubism SDK Framework
)

# 使用 file(GLOB_RECURSE ...) 自动查找 SDK 的所有 .cpp 源文件
# 这比手动一个个列出文件要方便得多
file(GLOB_RECURSE CUBISM_CORE_SOURCES "lib/Core/src/*.cpp")
file(GLOB_RECURSE CUBISM_FRAMEWORK_SOURCES "lib/Framework/src/*.cpp")

# --- 预处理器定义 ---
# 这些宏是配置 SDK 的关键
# CSM_TARGET_LINUX_GL: 告诉 SDK 我们的目标平台是 Linux + OpenGL
# CSM_ENABLE_DEBUG: 启用 SDK 内部的调试日志，对开发非常有用
add_definitions(
    -DCSM_TARGET_LINUX_GL
    -DCSM_ENABLE_DEBUG
)
# -------------------------

# --- 创建我们的可执行文件 ---
add_executable(AIPet main.cpp
    # 我们自己的 .cpp 文件
    src/WindowManager.cpp
    src/LAppAllocator.cpp
    src/LAppPal.cpp
    src/LAppTextureManager.cpp
    src/LAppModel.cpp
    src/LAppLive2DManager.cpp

    # AI 管理器 (暂时保留)
    src/AIManager.cpp
    
    # SDK 的 .cpp 文件
    ${CUBISM_CORE_SOURCES}
    ${CUBISM_FRAMEWORK_SOURCES}
)

# --- 链接所有需要的库 ---
target_link_libraries(AIPet PRIVATE 
    ${SDL2_LIBRARIES} 
    ${SDL2_IMAGE_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES} # 链接 GLEW
    ${CURL_LIBRARIES} 
    Threads::Threads
)

message(STATUS "Project configured for Linux with OpenGL and Cubism SDK.")