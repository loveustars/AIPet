cmake_minimum_required(VERSION 3.16)

project(AIPet CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# .a库是预编译的，不再需要C语言支持

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
    "lib/imgui/backends/imgui_impl_opengl3.cpp" # <-- 已修正拼写错误！
)
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC 
    lib/imgui 
    lib/imgui/backends
    ${SDL2_INCLUDE_DIRS}
)

# --- Cubism SDK 配置 (使用 .a 静态库) ---
set(CUBISM_SDK_DIR "lib/CubismSdkForNative")

# 只查找并编译 Framework 的源代码
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
    "${CUBISM_SDK_DIR}/Framework/src/Rendering/*.cpp"
    "${CUBISM_SDK_DIR}/Framework/src/Rendering/OpenGL/*.cpp"
)

# --- 预处理器定义 ---
add_definitions(-DCSM_TARGET_LINUX_GL -DCSM_ENABLE_DEBUG)

# --- 创建主程序可执行文件 ---
add_executable(AIPet main.cpp
    src/WindowManager.cpp
    src/AIManager.cpp
    src/Live2DManager.cpp
    ${CUBISM_FRAMEWORK_SOURCES}
)

# --- 为 AIPet 目标指定头文件搜索路径 ---
target_include_directories(AIPet PRIVATE
    src
    lib
    ${CUBISM_SDK_DIR}/Core/include
    ${CUBISM_SDK_DIR}/Framework/src
)

# --- 链接所有需要的库到主程序 ---
target_link_libraries(AIPet PRIVATE 
    # 使用 ${CMAKE_SOURCE_DIR} 构造绝对路径
    "${CMAKE_SOURCE_DIR}/${CUBISM_SDK_DIR}/Core/lib/linux/x86_64/libLive2DCubismCore.a"

    imgui_lib # <-- 链接 ImGui 库
    ${SDL2_LIBRARIES} 
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${CURL_LIBRARIES} 
    Threads::Threads
)

message(STATUS "Project configured to STATICALLY LINK against pre-compiled Cubism Core library.")