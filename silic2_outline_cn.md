# 3D FPS游戏开发规划文档
## Pixel Style Doom-like Game with Map Editor

### 项目概述
开发一款基于OpenGL的3D FPS单人游戏，风格类似于经典的Doom，但采用现代像素艺术风格（参考Steam上的Eclipsium）。游戏将包含自定义地图编辑器和高级粒子效果系统。

### 核心特性
- **3D FPS核心玩法**：第一人称射击，快节奏战斗
- **像素艺术风格**：低分辨率纹理配合现代光照效果
- **粒子效果系统**：烟雾、爆炸、魔法效果等
- **地图编辑器**：GUI界面，支持实时预览和测试
- **单人战役**：多个关卡，渐进式难度

### 技术栈

#### 核心引擎
- **图形API**: OpenGL 4.3+
- **编程语言**: C++ (主引擎) + Python (脚本/工具)
- **窗口管理**: GLFW
- **数学库**: GLM (OpenGL Mathematics)
- **图像加载**: stb_image
- **音频**: OpenAL 或 FMOD

#### 编辑器工具
- **GUI框架**: Dear ImGui (集成到主引擎)
- **文件格式**: JSON (关卡数据) + 自定义二进制格式 (优化版)

#### 构建系统
- **CMake** (跨平台构建)
- **vcpkg** 或 **Conan** (依赖管理)

### 项目结构

```
PixelFPS/
├── src/
│   ├── engine/           # 核心引擎
│   │   ├── renderer/     # 渲染系统
│   │   ├── physics/      # 物理/碰撞
│   │   ├── audio/        # 音频系统
│   │   ├── input/        # 输入管理
│   │   ├── particles/    # 粒子系统
│   │   └── core/         # 核心工具类
│   ├── game/            # 游戏逻辑
│   │   ├── player/      # 玩家控制
│   │   ├── weapons/     # 武器系统
│   │   ├── enemies/     # 敌人AI
│   │   └── world/       # 世界管理
│   ├── editor/          # 地图编辑器
│   │   ├── gui/         # 编辑器界面
│   │   ├── tools/       # 编辑工具
│   │   └── serialization/ # 数据序列化
│   └── tools/           # 额外工具
├── assets/              # 游戏资源
│   ├── textures/        # 纹理文件
│   ├── models/          # 3D模型
│   ├── sounds/          # 音效
│   ├── maps/            # 地图文件
│   └── shaders/         # 着色器
├── third_party/         # 第三方库
└── CMakeLists.txt
```

### 渲染系统设计

#### 像素风格实现
```cpp
// 核心渲染技术
- 低分辨率渲染目标 (320x200 或 640x400)
- 最近邻纹理过滤
- 有限调色板模拟
- 像素完美的UI渲染
```

#### 着色器系统
- **Vertex Shader**: 基础变换和光照计算
- **Fragment Shader**: 像素风格化处理
- **Compute Shader**: 粒子系统计算
- **Post-processing**: 屏幕空间效果

#### 光照系统
```glsl
// 简化的光照模型，适合像素风格
vec3 calculateLighting(vec3 worldPos, vec3 normal) {
    // 简单的方向光 + 点光源
    // 使用量化的颜色级别
    return quantizeColor(lighting, 8); // 8级量化
}
```

### 粒子效果系统

#### GPU粒子系统
```cpp
class ParticleSystem {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;
        float life;
        float size;
    };
    
    void update(float deltaTime);
    void render();
    void emit(const EmissionParams& params);
};
```

#### 效果类型
- **枪火效果**: 枪口闪光、弹壳抛射
- **爆炸效果**: 火焰、烟雾、碎片
- **环境效果**: 灰尘、蒸汽、魔法光效
- **UI效果**: 血液飞溅、击中指示器

### 地图编辑器规范

#### 核心功能
1. **3D视口**: 实时预览，支持飞行摄像机
2. **工具面板**: 画刷选择、纹理管理、实体放置
3. **层级管理**: 几何体、实体、光源分层
4. **实时测试**: 一键进入游戏模式测试

#### 编辑器界面布局
```cpp
// Dear ImGui 布局
┌─────────────┬─────────────────────┬─────────────┐
│  工具面板    │     3D视口          │  属性面板    │
│             │                     │             │
│ - 画刷工具   │                     │ - 选中对象   │
│ - 纹理库     │                     │ - 材质设置   │
│ - 实体库     │                     │ - 光照参数   │
│             │                     │             │
├─────────────┼─────────────────────┼─────────────┤
│  资源浏览器  │     时间轴/动画      │  输出日志    │
└─────────────┴─────────────────────┴─────────────┘
```

#### 地图数据格式
```json
{
  "version": "1.0",
  "worldSettings": {
    "gravity": -9.8,
    "ambientLight": [0.2, 0.2, 0.3]
  },
  "geometry": {
    "brushes": [
      {
        "id": 1,
        "vertices": [...],
        "faces": [...],
        "material": "wall_brick"
      }
    ]
  },
  "entities": [
    {
      "type": "player_start",
      "position": [0, 0, 0],
      "rotation": [0, 0, 0]
    }
  ],
  "lights": [
    {
      "type": "point",
      "position": [10, 5, 10],
      "color": [1.0, 0.8, 0.6],
      "intensity": 100
    }
  ]
}
```

### 游戏系统设计

#### 玩家控制系统
```cpp
class Player {
private:
    glm::vec3 position;
    glm::vec3 velocity;
    float pitch, yaw;
    float health;
    Weapon* currentWeapon;
    
public:
    void update(float deltaTime);
    void handleInput(const InputState& input);
    void takeDamage(float damage);
};
```

#### 武器系统
- **近战武器**: 拳头、链锯
- **射击武器**: 手枪、霰弹枪、机枪、火箭筒
- **特殊武器**: 等离子枪、BFG类型的武器

#### 敌人AI系统
```cpp
class Enemy {
public:
    enum State {
        IDLE,
        PATROL,
        ALERT,
        ATTACK,
        DEAD
    };
    
    void updateAI(float deltaTime, const Player& player);
    void takeDamage(float damage);
};
```

### 开发阶段规划

#### 第一阶段 (2-3周)：核心引擎
- [ ] OpenGL渲染环境搭建
- [ ] 基础着色器系统
- [ ] 纹理加载和管理
- [ ] 摄像机控制
- [ ] 输入系统

#### 第二阶段 (2-3周)：基础游戏系统
- [ ] 玩家控制器
- [ ] 基础物理/碰撞检测
- [ ] 简单的关卡加载
- [ ] 音频系统集成

#### 第三阶段 (3-4周)：地图编辑器
- [ ] Dear ImGui集成
- [ ] 3D视口实现
- [ ] 基础编辑工具
- [ ] 地图保存/加载系统

#### 第四阶段 (2-3周)：粒子效果
- [ ] GPU粒子系统
- [ ] 各种特效实现
- [ ] 性能优化

#### 第五阶段 (3-4周)：游戏内容
- [ ] 武器系统
- [ ] 敌人AI
- [ ] 关卡设计
- [ ] 音效和音乐

#### 第六阶段 (2-3周)：优化和完善
- [ ] 性能优化
- [ ] Bug修复
- [ ] 界面完善
- [ ] 最终测试

### 性能优化策略

#### 渲染优化
- **批量渲染**: 合并相同材质的几何体
- **视锥体剔除**: 只渲染可见物体
- **LOD系统**: 距离相关的细节级别
- **纹理图集**: 减少纹理切换

#### 内存管理
- **对象池**: 重复使用粒子、子弹等对象
- **资源流式加载**: 按需加载关卡资源
- **智能指针**: 自动内存管理

### 技术挑战和解决方案

#### 挑战1: 像素风格与现代光照的平衡
**解决方案**: 
- 使用低分辨率渲染目标
- 实现自定义的量化光照模型
- 保持像素完美的UI和HUD

#### 挑战2: 复杂粒子效果的性能
**解决方案**:
- GPU计算着色器处理粒子更新
- 实例化渲染减少draw call
- 智能的粒子生命周期管理

#### 挑战3: 编辑器的用户体验
**解决方案**:
- 参考成熟的编辑器UI设计 (Unity, Unreal)
- 实现撤销/重做系统
- 提供实时预览和快速测试功能

### 资源需求

#### 美术资源
- **纹理**: 64x64像素的墙面、地面纹理
- **Sprite**: 武器、敌人的2D精灵
- **UI元素**: 像素风格的界面组件

#### 音频资源
- **音效**: 枪声、爆炸声、脚步声
- **音乐**: 快节奏的电子音乐/金属音乐
- **环境音**: 机械声、怪物吼叫

### 部署和分发

#### 构建配置
- **Debug**: 开发和测试
- **Release**: 最终游戏版本
- **Editor**: 包含编辑器的完整版本

#### 平台支持
- **主要平台**: Windows, Linux
- **可选平台**: macOS (如果OpenGL兼容性允许)

### 预期成果

这个项目将产出：
1. **完整的3D FPS游戏**: 包含多个关卡的单人战役
2. **专业的地图编辑器**: 可视化编辑工具，支持mod制作
3. **可扩展的引擎**: 模块化设计，便于后续开发
4. **技术文档**: 完整的API文档和使用指南

### 开发工具推荐

#### IDE和编辑器
- **Visual Studio 2022** (Windows)
- **CLion** (跨平台)
- **VSCode** (轻量级开发)

#### 调试工具
- **RenderDoc**: OpenGL调试
- **Valgrind**: 内存泄漏检测 (Linux)
- **Visual Studio Debugger**: 集成调试

#### 版本控制
- **Git**: 代码版本管理
- **Git LFS**: 大文件资源管理

---

*这份文档为你的3D FPS游戏项目提供了完整的技术路线图。建议使用Claude Code来实现核心的C++代码部分，特别是渲染引擎和编辑器的复杂逻辑。项目的模块化设计允许你逐步实现各个系统，每个阶段都能产出可测试的成果。*

---

## Token使用量评估和Claude Code购买建议

### Token使用量预估：
- **核心引擎代码**：~50,000-80,000 tokens
- **编辑器系统**：~30,000-50,000 tokens  
- **游戏逻辑**：~20,000-30,000 tokens
- **调试和优化**：~20,000-40,000 tokens
- **总计**：约120,000-200,000 tokens

### 购买建议：
**推荐Claude Code Max**，因为：
1. 项目规模大，需要生成大量代码
2. 游戏开发需要频繁迭代和优化  
3. OpenGL和图形编程复杂度高
4. 建议先购买Max试用一个月，评估实际使用量

### 降低Token使用策略：
- 分阶段开发，模块化提问
- 建立代码模板，减少重复生成
- 增量式修改而非重写整个文件