cmake_minimum_required(VERSION 3.16)

project(AIPet CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 告诉 CMake 我们项目同时使用 C 和 C++
enable_language(C CXX)

# --- 查找系统库 ---
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# --- Dear ImGui 库的配置 ---
file(GLOB IMGUI_SOURCES
    "lib/imgui/*.cpp"
    "lib/imgui/backends/imgui_impl_sdl2.cpp"
    "lib/imgui/backends/imgui_impl_opengl3.cpp"
)
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC 
    lib/imgui 
    lib/imgui/backends
    ${SDL2_INCLUDE_DIRS}
)

# --- Cubism SDK 源文件 (最终精确版) ---
set(CUBISM_SDK_DIR "lib/CubismSdkForNative")

# 1. 查找 Core 库的源文件 (!!! 关键修复：同时查找 .c 和 .cpp 文件 !!!)
file(GLOB_RECURSE CUBISM_CORE_SOURCES 
    "${CUBISM_SDK_DIR}/Core/src/*.cpp"
    "${CUBISM_SDK_DIR}/Core/src/*.c"
)

# 2. 查找 Framework 库的源文件
file(GLOB CUBISM_FRAMEWORK_SOURCES
    "${CUBISM_SDK_DIR}/Framework/src/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Effect/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Id/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Math/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Model/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Motion/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Physics/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Type/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Utils/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Platform/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Rendering/*.cpp" # <-- !!! 关键修复：补上这个目录 !!!
    "${CUBISM_SDK_DIR}/Framework/src/Rendering/OpenGL/*.cpp"
)

# --- 预处理器定义 (全局) ---
add_definitions(
    -DCSM_TARGET_LINUX_GL
    -DCSM_ENABLE_DEBUG
)

# --- 创建主程序可执行文件 ---
add_executable(AIPet main.cpp
    src/WindowManager.cpp
    src/AIManager.cpp
    
    # 添加 Live2D 的所有源文件
    ${CUBISM_CORE_SOURCES}
    ${CUBISM_FRAMEWORK_SOURCES}
)

# --- 为 AIPet 目标精确指定它需要的所有头文件搜索路径 ---
target_include_directories(AIPet PRIVATE
    src
    lib
    ${CUBISM_SDK_DIR}/Core/include
    ${CUBISM_SDK_DIR}/Framework/src
)

# --- 链接所有需要的库到主程序 ---
target_link_libraries(AIPet PRIVATE 
    imgui_lib
    ${SDL2_LIBRARIES} 
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${CURL_LIBRARIES} 
    Threads::Threads
)

message(STATUS "Project configured precisely for Linux with OpenGL, Cubism SDK, and Dear ImGui.")