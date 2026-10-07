# 地图系统与编辑器升级

2026-10-07，用户授权同步修改 Silic2 地图模块与相邻的
`silic2_map_editor`。用户确认旧加载功能可用；本轮以保持旧地图可读和编辑后
游戏行为一致为基础，改善日常摆放、编辑及保存操作。

状态：**地图与编辑器实现完成，工程验收通过**。人工操作手感和样片试玩仍待验证。

## 设计约定

游戏和编辑器直接构建同一份无 OpenGL 依赖的 Map/JSON 源码。JSON 负责地图
布局、对象参数和资源引用；加载后生成供现有碰撞与渲染使用的 Brush。
保存不要求用户手工维护顶点或导航坐标。

- `schemaVersion: 2` 明确格式版本；没有该字段的旧地图走兼容读取。
- `gameMode` 决定玩法，`environmentPreset` 决定环境表现，材质名称只负责外观。
- `geometry.objects` 描述中心位置与尺寸的盒体或水平面；旧
  `geometry.brushes` 网格仍可读取和保存。两者共享唯一 ID 空间。
- 竞技场范围来自指定地板，障碍范围来自实际几何。裂隙和炮台位置来自地图
  实体，导航、导演、场景与小地图读取同一份已解析的数据。
- 加载到临时数据，完整验证后一次替换；失败保留原地图并返回具体错误。
- 已知字段严格验证，未知扩展字段保留，避免一次编辑丢失其他系统的数据。

本轮保留单层竞技场与现有碰撞规则。新盒体为轴对齐，水平面在 XZ 平面，
不提供任意旋转几何、模型导入、多层导航或新的游戏奖励系统。
当前竞技场导演仍要求一个玩家起点、四个裂隙槽和三个炮台槽。

## 编辑工作流

目标是打开地图后直接选取、查看并修改对象，同时保留场景视图。编辑操作
集中经过文档命令，统一维护选择、撤销历史和已保存状态。

- 对象列表与视口点选共享选择状态，选中对象有可见提示。
- 位置与尺寸直接编辑；提供吸附、聚焦、复制和删除。
- 按住鼠标右键浏览，松开恢复编辑；文本和数字输入不触发场景快捷键。
- 保存、另存、撤销与重做使用常见的 Cmd/Ctrl 快捷键。
- 新建、打开及退出遇到未保存修改时，提供保存、丢弃或取消选择。
- 新建竞技场从有效模板开始；旧地图可另存，原文件不会因验收被覆盖。

编辑器预览使用简单颜色、实体标记和选择轮廓，服务于布局编辑；游戏最终纹理、
光照、晶体特效与战斗仍需在游戏中查看。本轮没有加入游戏内热重载。

从游戏目录启动：

```sh
../silic2_map_editor/build/map_editor res/maps/starry_crystal_arena.json
./silic2-release /absolute/path/to/edited.json --seed 42
```

编辑器的文件浏览器提供 Game maps 入口；先选择模板或打开原地图，再另存自己的
文件。点击对象，使用 Position / Size 调整，F 聚焦，Cmd/Ctrl+D 复制，
Cmd/Ctrl+Z 撤销，Cmd/Ctrl+S 保存。完整操作见
[编辑器中文说明](../tools/map_editor/README_CN.md)。

## 验收清单

| 项目 | 完成条件 | 状态 |
| --- | --- | --- |
| 旧地图 | 仓库六张地图可读；保存再加载保持关键几何、实体、灯光和扩展字段 | 通过共享模块测试；五张原 legacy 文件 SHA 未变，旧样片有专用迁移测试 |
| 安全读写 | 重复加载不叠加；无效 JSON/版本/索引/尺寸返回错误且原地图不变 | 通过 Map/JSON 测试及编辑文档测试；地图与导航 ASan/UBSan 通过 |
| 单一地图数据 | 移动一个掩体，视觉、碰撞、导航和小地图同步；移动裂隙，生成与效果同步 | 通过共享数据测试、实际追击路线测试及环境 GL 检查 |
| 模式分离 | 更换地板材质不改变玩法；环境预设可独立选择 | 通过迁移/预设测试及实际 GL 检查 |
| 编辑命令 | 选择、变换、复制、删除、撤销、重做和保存点一致 | 3/3 CTest；旧图与竞技场 GL 合成输入均退出 0 |
| 防止丢失 | 打开失败保留文档；取消离开保留修改；保存成功才更新保存点 | 文档测试通过；真实 ImGui Cancel 点击与保存通过 |
| 两程序往返 | 编辑器输出实际由游戏加载，旧图和 v2 均覆盖 | 通过；逐字节核对编辑器输出与游戏输入副本，两次实际退出 0 |
| 运行与显示 | 编辑器实际 GL 界面及 Retina/尺寸检查；游戏流程和构建回归 | 两窗口尺寸通过；最终 Debug/Release 测试通过；默认及编辑后竞技场各 10/10 流程检查通过；三局性能回归通过 |
| 交付入口 | 用户原 `build/map_editor` 路径更新且可运行，使用说明与实际快捷键一致 | 已构建并核对最终源码/可执行文件 SHA；中英说明已更新 |

只将实际完成的检查标记通过。程序合成输入与原生桌面操作分别记录，不将前者
称为人类试玩；本轮也不替代两分钟样片的五人试玩门槛。

## 编辑器证据

两次实际 GL 运行分别编辑旧纹理地图和 v2 竞技场，覆盖点击、方向键、复制、
撤销、重做、未保存提示的 Cancel 按钮以及实际保存。1440×900 和 800×600
窗口均在 Retina 下运行，帧缓冲分别为 2880×1800 和 1600×1200。ATLAS 复核
了最终退出状态、保存内容与截图；早期失败的 smoke 不作为通过证据。

- [竞技场日志](acceptance/map-v2/editor-arena/smoke.log)、[实际保存地图](acceptance/map-v2/editor-arena/edited.json)
- [旧地图日志](acceptance/map-v2/editor-legacy/smoke.log)、[实际保存地图](acceptance/map-v2/editor-legacy/edited.json)
- [选中对象](acceptance/map-v2/editor-arena/frame-11.png)、[小窗口提示](acceptance/map-v2/editor-arena/frame-16.png)、[取消后保存](acceptance/map-v2/editor-arena/frame-22.png)
- [编辑器最终文件校验](acceptance/map-v2/editor-manifest.json)、[原地图完整性](acceptance/map-v2/original-map-integrity.json)
- [编辑器 CTest](acceptance/map-v2/editor-ctest.log)、[Debug 构建测试](acceptance/map-v2/build-debug.log)、[Release 构建测试](acceptance/map-v2/build-release.log)
- [游戏实际加载命令与退出码](acceptance/map-v2/final-smokes.json)、[默认流程](acceptance/map-v2/final-arena-flow/flow.jsonl)、[编辑后流程](acceptance/map-v2/final-editor-arena/flow.jsonl)
- [编辑后的竞技场在游戏中](acceptance/map-v2/final-editor-arena/flow-restart.png)、[旧地图的正确目标提示](acceptance/map-v2/final-editor-legacy/ready.png)
- [场景联动 GL 检查](acceptance/map-v2/environment/gl-validation.log)、[地图与导航 sanitizer 检查](acceptance/map-v2/map-sanitized.log)

这些检查注入的是合成键鼠事件，尚未验证人工操作手感；本轮运行平台为 macOS，
Windows/Linux 运行验证未执行。旧 `worldSettings.gravity` 保留为兼容字段，
不覆盖当前玩家的 `PlayerConfig.gravity`，编辑器已明确标注。

## 游戏集成与回归

旧地图按已有敌人的清场/探索模式显示 HUD；竞技场继续使用 90 秒敌潮规则。
异变文字与音效触发读取地图的环境开关和时间设置，避免保留固定的 45/49 秒
界面判断。射击与碰撞规则未在本轮重新设计，导航则改为读取真实地板和障碍。
实际 Chaser 测试覆盖移动中央障碍，以及整体平移、抬高、拉长后的竞技场。

首轮流程测试曾为 9/10：截图读回和写盘的首帧延迟使墙钟驱动的合成 Enter
先于 READY 检查。受控延迟复现后，让 flow 的合成输入与断言共享逐帧推进的
回放时钟，保留所有断言。同样延迟复测为 10/10；移除临时探针与延迟后，
最终默认地图及编辑后地图也都为 10/10。正常游戏与性能测量的墙钟不受此改动影响。

保留的诊断证据：[延迟复现失败](acceptance/map-v2/flow-window-before/flow.jsonl)、
[相同延迟下修复通过](acceptance/map-v2/flow-window-after/flow.jsonl)。

最终 Release 三局都完成 90 秒敌潮并清场，使用实际 GL 与音频设备。硬件为
M2 MacBook Air 16 GB；逻辑窗口 1280×720，Retina framebuffer 2560×1440，
场景内部分辨率 320×200，VSync 关闭。ATLAS 独立复算原始 CSV：

| 局数 | 战斗秒数 | 帧数 | p95 ms | p99 ms | 最大 ms | 最长连续 >50ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 90.004 | 43516 | 6.806 | 8.403 | 78.693 | 1 |
| 2 | 90.001 | 43472 | 6.792 | 8.338 | 68.935 | 1 |
| 3 | 90.001 | 44960 | 6.764 | 8.374 | 95.812 | 1 |

三局符合 p95 ≤20ms、p99 ≤33.3ms、无连续三帧 >50ms 的已定门槛。
存在孤立慢帧；合成移动、瞄准和无敌只用于工程测量，不能据此判断玩家难度。
[独立复算摘要](acceptance/map-v2/performance-audit.json) 保留命令、分辨率、
逐帧文件 SHA 与最终二进制 SHA；[游戏文件清单](acceptance/map-v2/game-manifest.json)
记录源码及可执行文件。最终地图联调与性能使用同一个 Release 二进制。
原始 CSV 和完整截图在本机 `artifacts/acceptance/map-v2`，代表证据随本页归档。

本轮没有重复上一轮的整套 50 次重开和满负载压力测试；它们属于
[历史样片验收](sample-acceptance.md)。本轮回归覆盖共享数据、编辑命令、实际导航、
环境联动、状态流程、实际游戏加载和三局完整运行。

需要复跑时，在游戏目录执行：

```sh
make -f Makefile.map BUILD=debug all test -j4
make -f Makefile.map BUILD=release all test -j4
ctest --test-dir ../silic2_map_editor/build --output-on-failure
./silic2-release --acceptance flow --output /tmp/silic2-map-flow-check
./silic2-release --vsync off --width 1280 --height 720 \
  --acceptance performance --output /tmp/silic2-map-performance-check
```

GL 检查顺序执行，避免同时运行编辑器与游戏影响性能测量。

## 保留与追溯

实施前备份路径：`/tmp/silic2-map-editor-before-20261007-214518`，包含 629 个
文件的 SHA-256 清单、源码副本及原编辑器可执行文件。游戏已有未提交改动
保留；编辑器原目录没有 Git 仓库，本轮不自动初始化或提交。

角色与唯一写入范围见 [团队索引](agent-team.md)。

## GitHub 源码交付

上传 GitHub 时，将已验收编辑器的构建所需源码和 ImGui 依赖纳入
`tools/map_editor`，供新克隆仓库使用。共享模块默认路径改为仓库根目录；
相邻原目录与可执行入口保留。后续编辑器源码以仓库内版本为维护入口。
仓库构建方法见 [主说明](../README_CN.md#地图编辑器)。本页上方的相邻目录命令
记录原验收环境；新克隆无需该外部目录。

仓库内版本已独立 CMake 构建，3/3 CTest 通过，
[日志](acceptance/map-v2/editor-repository-ctest.log) 已保留；交互实现与原验收版本
逐字节一致。Dear ImGui 仅导入所需的 16 个源码/头文件/许可证文件（约 3.1 MB），
实际版本、来源与 SHA 见 [依赖记录](../tools/map_editor/DEPENDENCIES.md)。
