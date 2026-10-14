# Silic2：Hollow Orbit 太空恐怖竞技场

宗旨：在异变空间中高速移动、主动瞄准，通过短场战斗获得改变打法的武器能力。

当前实现的是两分钟样片：36×36 太空遗构竞技场、现有主武器、追击者与蓄力炮台。
表现方向是受活体侵蚀的巨大遗构：黑暗空域、骨化障碍、中空生物与粗粝工业声景。
90 秒停止生成敌人，清场获胜；第 45 秒开始一次四秒视听异变。
本阶段验证移动、主动瞄准、威胁识别和短局节奏，武器能力奖励留到下一里程碑。

Crystal 现为五区连通的立体战斗地图：受晶体侵入的轨道巨构，包含中央裂环庭、
桥下晶壳沟谷、上层弧道、倾柱侧庭和天空侧廊。宽坡连接上下层，桥下可以真实
通行；曲面和斜撑使用可复用模型及三角形碰撞。沿用 90 秒敌潮＋清场，不再有
观察对齐解谜。小地图保持完整显示，异变只改变表现。
两张地图共用当前敌人、武器和音效；Hollow Orbit 仍是默认入口。
范围见[确认方案](plan/crystal-combat-world.md)，验证见
[立体地图验收](docs/crystal-combat-acceptance.md)，格式见
[空间结构合同](docs/crystal-spatial-contract.md)。
Crystal 已接入四类原创64×64表面贴图，素材、提示词和前后对照见
[贴图改造记录](docs/crystal-texture-pass.md)。

当前美术改造见[太空恐怖验收记录](docs/cosmic-horror-acceptance.md)，
原样片测试与人类试玩待办见[验收记录](docs/sample-acceptance.md)，范围以
[样片计划](plan/two-minute-sample.md)为准，协作入口见[团队索引](docs/agent-team.md)。

## GitHub 资源规则

GitHub 只交付源码、地图 JSON、shader、生成脚本、资源清单、提示词和文字验收记录。
模型、材质贴图、音频、截图和视频留在本地；构建产物也不上传。第三方构建依赖保留。
历史验收文档中的图片链接及资源哈希对应本地完整版本，源码克隆不包含这些媒体文件。
已有远端历史不在本次交付中重写，因此历史提交仍可能包含早期资源。

新克隆仓库先在根目录生成 Crystal 必需模型及音频，再构建、运行或测试：

```bash
python3 tools/environment/generate_crystal_combat.py
python3 tools/audio/generate.py
```

四张 AI 贴图不由脚本重建；需从本地资源备份补入 `res/texture/crystal_combat/`，
缺失时 Crystal 使用程序化材质回退。其他旧图引用的贴图也需自行补齐。
资源文件已加入 `.gitignore`，后续提交不要使用 `git add -f` 纳入它们。

## 构建与启动

需要 C++17 编译器、Make、GLFW 和 OpenGL 3.3。macOS 使用 Apple Clang，
GLFW 可通过 Homebrew 安装；Windows 保留 MinGW-w64 和仓库库文件配置；
Linux 需要 GLFW/OpenGL 开发库。当前运行验收针对 M2 Mac，其他平台待验证。

请在仓库根目录运行，确保相对资源路径正确：

```bash
make -f Makefile.map -j8
./silic2

make -f Makefile.map BUILD=release -j8
./silic2-release --seed 42

# Crystal：五区连通、真实坡道与上下层战斗
./silic2-release res/maps/starry_crystal_arena.json --seed 42

# 太空恐怖竞技场（也是默认地图）
./silic2-release res/maps/hollow_orbit_arena.json --seed 42

# 显式加载旧地图
./silic2-release res/maps/test_room.json --seed 42

make -f Makefile.map BUILD=debug test
make -f Makefile.map BUILD=release test
```

Windows 可执行文件带 `.exe`。默认地图是 `res/maps/hollow_orbit_arena.json`，
默认 seed 为 42，VSync 开启。场景分辨率 320×200，HUD 在后处理之后按逻辑
窗口尺寸布局、在 Retina framebuffer 绘制。可用 `--width 800 --height 600`
调整启动尺寸，`--vsync off` 关闭垂直同步，`--help` 查看命令。

## 操作

| 操作 | 按键 |
| --- | --- |
| READY 开始 | Enter |
| 移动 / 瞄准 | WASD / 鼠标 |
| 开火 | 按住左键 |
| 切换奔跑 | Shift |
| 蹲下 / 滑铲 | Ctrl |
| 跳跃 | Space |
| 暂停 / 恢复 | Esc；失去焦点自动暂停 |
| 死亡 / 胜利后同 seed 重开 | 等待一秒后按 R 或重新单击左键 |

READY 阅读时间不计入战斗；暂停冻结战斗与表现。按住开火不能跳过结算锁。
保留开发用 G 自由飞行/无碰撞开关，正式试玩和正常验收不要启用。
本阶段没有副武器、拾取、奖励构筑或 Boss。

## 地图编辑器

独立编辑器源码已纳入 `tools/map_editor`，克隆本仓库后即可构建。它与游戏共享
地图读写和空间模块，支持旧地图、原生坡道、静态 OBJ 模型导入、变换/吸附/复制、
碰撞及导航显示。编辑后另存，再从游戏加载该 JSON。
本机原来的相邻 `silic2_map_editor/build/map_editor` 入口仍然保留。

```bash
cmake -S tools/map_editor -B tools/map_editor/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build tools/map_editor/build -j4
./tools/map_editor/build/map_editor res/maps/starry_crystal_arena.json
./silic2-release /absolute/path/to/your-map.json --seed 42
```

编辑器构建与快捷键见[编辑器使用说明](tools/map_editor/README_CN.md)；
格式见[地图格式](docs/map-format.md)及[v3 空间合同](docs/crystal-spatial-contract.md)。
另存会重写相对资源路径，但不会复制模型；分享地图时需要同时携带依赖的 OBJ。

## 可复现工程验收

```bash
./silic2-release res/maps/starry_crystal_arena.json --acceptance spatial-combat --vsync off --output /tmp/silic2-crystal-combat
./silic2-release --acceptance screenshots --output /tmp/silic2-screens
./silic2-release --acceptance flow --output /tmp/silic2-flow
./silic2-release --acceptance restarts --output /tmp/silic2-restarts
./silic2-release --acceptance performance --vsync off --output /tmp/silic2-perf
./silic2-release --acceptance stress --vsync off --output /tmp/silic2-stress
```

这些入口使用明确标记的合成输入，用于实际 GL、音频、生命周期和性能检查，
不代表玩家的命中率、难度或学习体验。保持窗口聚焦；性能测量时避免其他 GPU
预览及编译。输出 JSONL 和帧耗时 CSV；普通局默认写入 `artifacts/runs`，验收
默认写入 `artifacts/acceptance/<模式>`。50 次全系统重开不能由单模块 reset 替代。
五人试玩和耳机听感需要实际观察，记录模板在验收文档中。

## 模块与资产

- `src/engine`：主循环、地图、碰撞、渲染基础及验收入口。
- `src/game`、`src/enemy`、`src/player`：导演、导航、敌弹、统计及原有玩家/武器。
- `src/hud`：原创批量 5×7 字体、HUD、菜单及结算。
- `src/environment`、`src/character`：程序化遗构、异常天体、骨化场景与角色武器表现。
- `src/audio`：24 声部混音，8 个关键提示保留声部，预加载和空间播放。
- `res/audio`、`tools/audio`：13 个原创 WAV 及离线生成器；运行时不需要 Python。

音频依赖固定 miniaudio 0.11.25，采用 MIT No Attribution 选项；原文与校验见
[依赖来源](third_party/miniaudio/PROVENANCE.md)。GLFW、GLM、GLAD、stb_image 的
说明随源码保留。旧截图在 `res/screenshot`。
