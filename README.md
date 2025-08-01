# Silic2 - 像素风格FPS游戏

一个使用OpenGL实现的像素艺术风格第一人称射击游戏，采用Doom风格的快节奏战斗和复古3D视觉效果。

## 游戏特色

### 🎮 游戏体验
- **复古像素美学** - 低分辨率渲染营造90年代FPS游戏氛围
- **快节奏战斗** - Doom风格的流畅移动和激烈战斗
- **关卡编辑器** - 内置地图编辑器，支持实时预览和测试
- **多样化武器** - 近战武器、火器和特殊武器系统

### 🔧 技术特性
- **OpenGL 4.3+渲染** - 现代图形API实现复古视觉效果
- **像素完美渲染** - 320x200/640x400低分辨率渲染目标
- **粒子效果系统** - GPU计算着色器驱动的爆炸、烟雾、枪口闪光等效果
- **智能AI系统** - 敌人状态机AI (巡逻、警戒、攻击、死亡)
- **JSON地图格式** - 灵活的关卡数据存储和加载

## 编译和运行

### 环境要求
- **操作系统**: Windows 10/11
- **编译器**: MinGW-w64 (GCC)
- **图形API**: OpenGL 4.3+
- **构建系统**: Make或CMake

### 快速开始
```bash
# 编译游戏
make -f Makefile.map

# 运行
./silic2.exe

# 选择地图
./silic2.exe res/maps/textured_room.json 
./silic2.exe res/maps/test_room.json
```

## 游戏控制

### 基础操作
- **WASD** - 角色移动
- **鼠标移动** - 摄像机视角控制
- **鼠标左键** - 开火/攻击
- **鼠标右键** - 瞄准/副武器
- **滚轮** - 切换武器
- **空格** - 跳跃
- **Shift** - 奔跑
- **Ctrl** - 蹲下

### 编辑器操作
- **F1** - 切换编辑器界面
- **鼠标中键拖拽** - 平移视图
- **Alt + 鼠标** - 旋转摄像机
- **Ctrl + S** - 保存地图
- **Ctrl + O** - 打开地图

## 项目结构
```
silic2/
├── src/
│   ├── engine/          # 核心引擎代码
│   │   ├── renderer/    # 渲染系统
│   │   ├── particles/   # 粒子效果系统
│   │   └── audio/       # 音频系统
│   ├── game/            # 游戏逻辑
│   │   ├── player/      # 玩家控制器
│   │   ├── weapons/     # 武器系统
│   │   └── enemies/     # 敌人AI
│   └── editor/          # 地图编辑器
├── res/
│   ├── shaders/         # GLSL着色器文件
│   ├── textures/        # 64x64像素材质
│   ├── maps/            # JSON格式地图文件
│   └── audio/           # 音效和音乐
├── include/             # 头文件
├── lib/                 # 静态库 (GLFW, GLM等)
└── bin/                 # 编译输出
```

## 技术架构

### 渲染管线
- **低分辨率目标** - 像素完美的复古视觉效果
- **量化光照** - 8级光照量化模拟早期3D游戏
- **批量渲染** - 优化的几何体渲染性能
- **视锥剔除** - 提升大型关卡渲染效率

### 游戏系统
- **实体组件系统(ECS)** - 灵活的游戏对象管理
- **对象池** - 粒子、子弹等临时对象的内存优化
- **智能指针** - 自动内存管理避免内存泄漏
- **观察者模式** - 事件驱动的系统通信

### 开发阶段
1. ✅ **核心引擎** - 渲染、输入、摄像机系统
2. 🔄 **基础游戏系统** - 玩家控制、物理、音频
3. 📋 **地图编辑器** - 编辑界面和功能实现
4. 📋 **粒子效果** - 视觉效果和动画系统
5. 📋 **游戏内容** - 武器、AI、关卡设计
6. 📋 **优化打磨** - 性能优化和用户体验

## 贡献指南

欢迎提交Issue和Pull Request！请确保：
- 遵循现有的代码风格
- 添加适当的注释和文档
- 测试新功能和修复

## ScreenShots

- ScreenShots在 res\screenshot 里面，仅供参考

## 许可证

本项目采用MIT许可证 - 详见 [LICENSE](LICENSE) 文件

## 致谢

- **GLFW** - 窗口和输入管理
- **GLM** - OpenGL数学库
- **stb_image** - 图像加载
- **Dear ImGui** - 编辑器用户界面
- **Claude Code** - 部分代码支持