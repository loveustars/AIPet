# AIPet
A gemini based chatting agent


鉴于Cubism未声明arm64的Ubuntu支持，本程序**在amd64架构的VMware Ubuntu 虚拟机测试**，实体机效果未知。

### 1. 安装基础编译环境和核心库

打开终端，执行以下命令来安装 CMake 构建工具、C++ 编译器以及本项目所需的 `SDL2`, `libcurl` 和 `FreeType` 库。

```bash
# 更新软件包列表
sudo apt update
```

```bash
# 安装基础编译工具
sudo apt install build-essential g++ cmake
```

```bash
# 安装 libcurl，用于网络通信
sudo apt install libcurl4-openssl-dev
```

```bash
# 安装 SDL2，用于创建窗口和处理输入
sudo apt install libsdl2-dev
```

```bash
# 安装 FreeType，用于字体渲染
sudo apt install libfreetype-dev
```

### 2. 准备第三方库 (仓库中已经有了)

**项目结构约定:**
```
main/
├── lib/
│   ├── CubismSdkForNative/  # Live2D SDK
│   ├── dear_imgui/          # Dear ImGui 源码
│   ├── stb/                 # 官方建議的图形解析头文件
│   └── nlohmann/            # nlohmann/json.hpp
├── src/
│   ├── *.cpp
│   ├── *.hpp
│   ...
│   ...
├── assets/
│   ├── fonts/               # 放入文字
│   ├── live2d_models/       # live2d模型
│   │   ├── hiyori_pro_t11
│   │   └── Haru
│   └── ...
│
│
│   ...
├── CMakelists.txt
├── main.cpp
└── ... 
```

**操作步骤:**

1. 首先，克隆此仓库到本地。
```bash
git clone https://github.com/loveustars/AIPet.git
```

2. 然后，进入项目根目录，进行编译。
```bash
rm -rf build  
mkdir build && cd build  
cmake ..
make
# 在运行之前，将临时的环境变量添加到系统，用于程序获得与大模型的API.
export GOOGLE_AI_STUDIO_API_KEY='此处替换为你申请的API'

# 以防路径问题，请在bin文件夹直接进行运行。
cd bin
./AIPet
```

---

### 已知问题：
1. 当前模型窗口以及对话窗口几乎叠在一块并且无法拖拽，还没找到解决方案。
2. 长时间将鼠标焦点置于窗口外部时，闲置后有相当大的概率卡死。
3. 模型加载，字体选择都是硬编码在源代码中，使用不便利。

---

### 注意事项:
1. 对于Apple Silicon电脑运行的arm64 Ubuntu虚拟机，**不支持** CubismSdkForNative，考虑运行云端x86的Ubuntu 或 本地QEMU虚拟（不推荐）。
2. 若希望实现对话功能，**务必**访问本项目使用的[大模型网页](https://aistudio.google.com)，若跳转为Available Regions...，证明你的网络环境不行，你需要成为魔法少女。
3. 对于API获取，进入Google AI Studio后有Get API Key按钮，通俗易懂的界面，[点击这里也可以进入](https://aistudio.google.com/app/apikey?hl=zh-cn)，不赘述。

---

## 常见问题(待扩充)

**Q: AI说了一大长串带好几个花括号的话， 还有什么数字的事，这家伙在说什么呢？**  

A: 取决于数字是什么。400——说明API过期，需要更新；  403——你的API被封锁了，很严格的封锁，这种情况建议
**重新申请**一个API，如果是因为你试图引导AI说奇怪的话，~~那你纯属魔丸~~ ；  502——你选错了魔法，可以
尝试吟唱其他魔法；  429——话太多了，到达了使用上限，明早起来再试试；  其他错误数字正在发掘。

**Q: 模型不见了？**

A: 检查bin文件夹里有没有assets文件夹被自动复制过去，模型名是否正确。

**Q: 终端报错 vert Shaders...$$$? fonts ... $$$?**

A: 有没有在bin文件夹执行？如果有那就得把根目录的复制过来。没有的话那就进bin再执行。

**Q: 窗口加载失败？**

A: 可能的错误太多了导致我不知道写什么，发在issue里可能会更好。
