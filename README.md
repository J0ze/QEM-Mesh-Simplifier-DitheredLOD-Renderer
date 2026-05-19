# Dynamic QEM LOD Renderer 🚀

> [English Version Below](https://www.google.com/search?q=%23english-version)

这是一个基于 **QEM（二次误差度量，Quadric Error Metrics）** 算法与现代 OpenGL 渲染管线的高性能 3D 模型细节层次（LOD）简化与渲染系统。本项目针对复杂拓扑网格在简化时容易产生的崩溃拉丝问题，独创了**顶点版本戳（Vertex-Versioning）机制**；并在片元着色器中引入 Bayer 矩阵，实现了 3A 级别的 **Dithered LOD（抖动透明）无缝平滑过渡**。

---

## 🌟 核心特性 (Features)

* **进阶 QEM 拓扑简化**：基于 C++ 实现了 QEM 边折叠算法。独创的**顶点版本号追踪机制**有效解决了传统静态优先队列导致的历史“脏数据”问题，完美消除了复杂非水密（Non-watertight）模型的高频拉丝与破洞现象。
* **显存友好的统一 VBO 架构**：通过几何与拓扑数据解耦，6 个 LOD 层级（100%~25%）共享同一个统一顶点缓冲对象（Unified VBO），仅在运行时轻量级切换索引缓冲（EBO），实现极低的显存占用与零带宽切换。
* **无缝 Dithered LOD 过渡**：在 Fragment Shader 中引入 $4 \times 4$ Bayer 矩阵进行屏幕空间剔除。利用互斥逻辑开关（`u_isInverted`）实现新旧模型的无缝交叉溶解，无需 Alpha 排序，彻底杜绝 Z-Fighting 深度闪烁。
* **高鲁棒性的平面深度调度**：采用基于摄像机 Z 轴绝对深度的 LOD 调度机制，大幅降低 CPU 开方运算开销，并完美消除了因水平视差平移导致的 LOD 边界反复抽搐。

---

## 🛠 环境依赖 (Dependencies)

本项目基于现代 OpenGL (Core Profile) 开发，请确保您的开发环境（如 Visual Studio 或 CMake）已配置以下库：

* **[GLFW](https://www.google.com/search?q=https://www.glfw.org/)** (用于窗口管理与输入控制)
* **[GLAD](https://www.google.com/search?q=https://glad.dav1d.de/)** (OpenGL 函数指针加载)
* **[GLM](https://www.google.com/search?q=https://glm.g-truc.net/0.9.9/index.html)** (OpenGL 数学库)
* **[Assimp](https://www.google.com/search?q=https://github.com/assimp/assimp)** (用于导入 `.obj`, `.fbx`, `.pmx` 等复杂 3D 模型格式)
* **stb_image.h** (项目中已包含，用于纹理贴图加载)

---

## 📦 编译与运行 (Build & Run)

### Visual Studio 环境配置

1. 克隆本仓库到本地。
2. 在 Visual Studio 中创建一个新的空 C++ 项目，并将仓库中的所有 `.h` 和 `.cpp` 文件添加至工程。
3. 配置项目的 **包含目录（Include Directories）** 和 **库目录（Library Directories）**，使其指向 GLFW、GLAD、GLM 和 Assimp 的路径。
4. 在 **链接器 -> 输入** 中添加 `glfw3.lib`, `assimp.lib`。
5. 编译并运行项目。

---

## 📖 使用指南 (Usage)

### 1. 导入模型

系统默认支持 Assimp 可解析的绝大多数 3D 格式。在主程序或初始化逻辑中，通过实例化 `LODModel` 类即可自动加载并触发多级简化（支持保留率 100%, 75%, 50%, 25% 等）。

```cpp
// 创建一个 LOD 模型实例，路径指向你的模型文件
// 系统会在初始化时自动根据 QEM 算法计算并生成所有 LOD 级别的 EBO 索引数据
LODModel ourModel("resources/objects/bunny/bunny.obj");

```

### 2. 摄像机控制

* `W` `A` `S` `D`：控制摄像机在空间中的移动（前后左右平移）。
* `鼠标滑动`：控制摄像机的 Pitch 与 Yaw 视角旋转。
* **观察 LOD 平滑过渡**：当使用 `W` 和 `S` 键前后拉近/推远摄像机时，跨越预设的距离阈值即可触发 Dithered LOD 抖动溶解效果。

---

## 📄 开源协议

本项目采用 [CC-BY-NC-SA 4.0](https://www.google.com/search?q=https://creativecommons.org/licenses/by-nc-sa/4.0/) 协议开源。欢迎学习交流，但在未授权情况下严禁用于商业用途。







---

---

# Dynamic QEM LOD Renderer 🚀

A high-performance 3D mesh simplification and Level of Detail (LOD) rendering system based on the **Quadric Error Metrics (QEM)** algorithm and modern OpenGL. This project introduces an innovative **Vertex-Versioning mechanism** to resolve topology artifacts in complex meshes and implements a 3A-grade **Dithered LOD smooth transition** via a Bayer matrix in the Fragment Shader.

---

## 🌟 Features

* **Advanced QEM Simplification:** Custom C++ implementation with a unique **Vertex-Versioning tracking mechanism** to eliminate static priority queue stale data. This perfectly eradicates geometry popping, stretching, and hole artifacts, especially on complex non-watertight models.
* **Memory-Efficient VBO Architecture:** Decouples geometry from topology. All 6 LOD levels (from 100% to 25% retention) share a **Unified Vertex Buffer Object (VBO)**. The system toggles only lightweight Index Buffers (EBO) at runtime, achieving near-zero VRAM overhead and bandwidth switching costs.
* **Seamless Dithered LOD:** Implemented screen-door transparency using a $4 \times 4$ Bayer Matrix in the Fragment Shader. A mutually exclusive logic toggle (`u_isInverted`) ensures seamless cross-fading between old and new models without Alpha-sorting overhead or Z-Fighting depth artifacts.
* **Robust Planar-Depth Scheduling:** Utilizes a camera Z-axis absolute depth trigger mechanism, drastically reducing CPU scalar calculation (square root) overhead and completely eliminating LOD boundary flickering caused by lateral camera panning.

---

## 🛠 Dependencies

Developed with modern OpenGL (Core Profile). Please ensure your environment is configured with the following libraries:

* **[GLFW](https://www.google.com/search?q=https://www.glfw.org/)** (Window management and input)
* **[GLAD](https://www.google.com/search?q=https://glad.dav1d.de/)** (OpenGL function pointer loader)
* **[GLM](https://www.google.com/search?q=https://glm.g-truc.net/0.9.9/index.html)** (OpenGL mathematics)
* **[Assimp](https://www.google.com/search?q=https://github.com/assimp/assimp)** (Asset Import Library for `.obj`, `.fbx`, etc.)
* **stb_image.h** (Included in the project for texture loading)

---

## 📦 Build & Run

### Visual Studio Setup

1. Clone the repository.
2. Create a new empty C++ project in Visual Studio and add all `.h` and `.cpp` files.
3. Configure the **Include Directories** and **Library Directories** to point to your GLFW, GLAD, GLM, and Assimp installations.
4. Add `glfw3.lib` and `assimp.lib` to your **Linker -> Input -> Additional Dependencies**.
5. Build and run.

---

## 📖 Usage Guide

### 1. Importing Models

The system supports most formats parsed by Assimp. Instantiate the `LODModel` class in your main logic, and the system will automatically parse the model, calculate QEM matrices, and generate multiple discrete LOD levels (e.g., 100%, 75%, 50%, 25%).

```cpp
// Create an LOD model instance pointing to your asset path.
// The system automatically executes QEM and caches all Multi-EBO index data upon initialization.
LODModel ourModel("resources/objects/bunny/bunny.obj");

```

### 2. Camera Controls

* `W` `A` `S` `D`: Move the camera forward, backward, left, and right.
* `Mouse Movement`: Control camera Pitch and Yaw.
* **Observe Smooth LOD Transitions:** Use `W` and `S` to move the camera closer to or further from the model. Crossing the predefined distance thresholds will trigger the seamless Dithered LOD cross-fade effect.

---

## 📄 License

This project is licensed under the [CC-BY-NC-SA 4.0](https://www.google.com/search?q=https://creativecommons.org/licenses/by-nc-sa/4.0/) License. Free for academic and educational purposes. Commercial use is strictly prohibited without permission.
