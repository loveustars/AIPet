# AIPet
A gemini based chatting agent
鉴于Cubism未声明arm64的Ubuntu支持，本程序在amd64架构的Ubuntu测试

### 1. 安装基础编译环境和核心库

打开终端，执行以下命令来安装 CMake 构建工具、C++ 编译器以及本项目所需的 `SDL2`, `libcurl` 和 `FreeType` 库。


# 更新软件包列表
```bash
sudo apt update
```

# 安装基础编译工具
```bash
sudo apt install build-essential g++ cmake
```

# 安装 libcurl，用于网络通信
```bash
sudo apt install libcurl4-openssl-dev
```

# 安装 SDL2，用于创建窗口和处理输入
```bash
sudo apt install libsdl2-dev
```

# 安装 FreeType，用于字体渲染
```bash
sudo apt install libfreetype-dev
```

### 2. 准备第三方库 (手动下载)

部分库需要我们手动下载并放置在项目目录中。

**项目结构约定:**
```
ai-desktop-pet/
├── lib/
│   ├── cubism_sdk_native/   # 在此放入 Live2D SDK
│   ├── dear_imgui/          # 在此放入 Dear ImGui 源码
│   └── nlohmann/            # 在此放入 nlohmann/json.hpp
├── src/
|   ├── WindowManager.hpp
|   ├── WindowManager.cpp    # 待重构为纯OpenGL渲染
|   ├── AIManager.hpp
|   ├── AIManager.cpp        # 无需重构
|   ...
├── assets/
|   ├── fonts/               # 放入文字（待重构）
|   ├── live2d_models/       # live2d模型
|   ...
├──
└── ... 
```

**操作步骤:**

1.  **nlohmann/json (用于解析JSON)**
    ```bash
    # 进入你的项目根目录
    cd ai-desktop-pet
    # 创建 lib 目录
    mkdir -p lib
    # 从 GitHub 克隆 nlohmann/json 库
    git clone https://github.com/nlohmann/json.git lib/nlohmann
    ```

2.  **Dear ImGui (用于UI界面和文本渲染)**
    ```bash
    # 确保在项目lib
    # 从 GitHub 克隆 Dear ImGui 库
    git clone https://github.com/ocornut/imgui.git lib/dear_imgui
    ```

3.  **Live2D Cubism SDK for Native**
    *   请前往 [Live2D 官方网站](https://www.live2d.com/en/sdk/download/cubism/) 下载 "SDK for Native"。
    *   注意使用个人版。
    *   解压下载的文件。
    *   将其中的 `Core` 和 `Framework` 文件夹复制到你项目中的 `lib/cubism_sdk_native/` 目录下。
