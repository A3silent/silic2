# Hollow Orbit：太空恐怖表现改造

2026-10-08，按用户新的风格方向实施。依据与参考来源见
[美术方向](../plan/cosmic-horror-art-pass.md)。本轮在原有两分钟样片上调整表现，
保留快速移动、主动瞄准、原攻击/碰撞/敌潮规则和地图编辑兼容性。

状态：第一轮表现改造已完成，最终游戏集成与自动化工程检查通过；人工试玩待完成。

后续双地图更新：用户要求同时保留太空恐怖与原水晶竞技场，敌人改动全部保留。
默认恐怖图使用 `res/maps/hollow_orbit_arena.json` / `hollow_orbit`，水晶图使用
`res/maps/starry_crystal_arena.json` / `starry_crystal`。下面的第一轮验收数据对应
拆分前版本；双地图版本的验证结果单独记录于文末。

## 本轮变化

- 场景：稀疏黑空、受遮蔽天体、场外巨大断裂肋架；中央及侧掩体使用骨化矿壳、
  断缝与局部苍白反射；地板保持可辨的跑动范围。装饰从实际地板范围派生，
  包括极窄长地图在内的外围几何检查要求全部留在可玩范围之外。
- 怪物：高窄非对称中空肋笼追击者；低矮骨杯器官炮台，偏心凹孔与不等长壳瓣。
  充能/锁定读取实际状态，身体外接范围保持原契约；武器改为磨损管状机具和骨质嵌件。
- 界面：骨白、炭灰、旧琥珀色，竞技场显示 HOLLOW ORBIT。原字体、布局、
  地图可见性过滤、结果锁和输入机制保持；低血量与受伤方向继续明确提示。
- 音频：8 个原创声音重制，低频金属/膜片、器官腔体噪声、粗粝预警及克制结果声。
  13 个 cue 的接口与时长不变；5 个混音增益调整，预警音量与优先级保留。

首轮场景图的障碍过于接近黑色，审阅后增强少量骨化面的局部反射和接触边界，
保留大暗部；同视角中央区域 p95 显示亮度由 27.28/255 调整至 51.01/255，
中位数仍为 10/255。这是定位修改用的截图统计，不是恐怖感或可读性的人测分数。
炮台第一版正面过于规则，随后增加偏心孔、不等骨瓣与少量内壁组织色。

## 工程证据

| 检查 | 状态与范围 |
| --- | --- |
| 场景 | 最终 CPU 几何、shader 链接、实际 GL、暂停/重置同像素、旧图切换通过；同机位异变前中后已复核 |
| 角色 | 10,240 组动画/朝向/充能/受击几何检查通过；最终炮台修订六姿势GL、状态恢复、世界深度及50次表现重置通过 |
| 界面 | Debug/Release HUD 测试、前后 8 状态×2尺寸 GL 各一轮通过；Retina 及低 HP 截图已复核 |
| 音频 | Debug/Release 离线混音、13 资产重新合成一致性、PCM/峰值/时长/循环检查通过 |
| 游戏集成 | Debug/Release 全套测试通过；14张整合GL截图无错误且冻结同像素；实际游戏默认flow 10/10及旧地图加载退出0，真实音频可用 |
| 性能 | 最终 Release 三局独占测量通过；ATLAS 从原始逐帧 CSV 独立复算，程序与资源校验一致 |

性能配置：M2 MacBook Air 16GB，逻辑窗口 1280×720、Retina 帧缓冲 2560×1440、
内部渲染 320×200、VSync 关闭、真实音频开启；合成输入，每局完整 90 秒战斗。

| 局数 | 帧数 | p95 / ms | p99 / ms | 最大帧耗时 / ms | 连续 >50ms 最多帧数 |
| --- | --- | --- | --- | --- | --- |
| 1 | 40,942 | 6.68 | 10.69 | 279.67 | 2 |
| 2 | 42,433 | 6.37 | 8.15 | 302.81 | 2 |
| 3 | 41,239 | 6.65 | 8.49 | 138.97 | 1 |

三局均满足 p95≤20ms、p99≤33.3ms、无连续三帧>50ms 的既定门槛。
仍存在孤立长帧；原日志保留五次失焦后的自动恢复，不删除帧记录或归因为已证实原因。
通过此门槛不代表没有卡顿。见 [独立性能复算](acceptance/horror/performance-audit.json)
及 [运行记录](acceptance/horror/performance/verification.json)。本轮未重复原样片的
全局压力及50次整局重开；其历史结果不能当作新版本新增验收。

场景静态模块为 5,832 顶点、1,944 三角形，约 160 KiB VBO；示例地图仍为
8 个 brush 加 sky/structure/points 共 11 次场景绘制。角色保持世界/第一人称
两次绘制与 256 效果容量；当前最坏世界顶点为 62,964/65,536。

音频离线叠加环境和异变时，15 单位距离的左右能量差分别为裂隙 8.184 dB、
炮台蓄力 11.9602 dB；10 秒立体声预听 peak 0.285208。首轮异变声遮蔽过强，
降低其增益后复测通过。上述数字验证路由与能量关系，不能替代耳机辨位观察。
详见 [声音验证](../tools/audio/HORROR_VALIDATION.md)；
[10 秒预听](../tools/audio/previews/cosmic-horror-mix.wav) 使用实际运行时混音器离线生成。

## 代表画面

以下是实际 GL 帧缓冲截图。环境使用真实地图；角色和界面模块预览使用构造快照。

![整合渲染：构造战斗快照，非人类试玩](acceptance/horror/presentation/charge-lock-45-1280.png)

- [原环境同视角](acceptance/horror/env/baseline/spawn-before.png) →
  [新环境](acceptance/horror/env/spawn-before.png)，以及
  [异变后](acceptance/horror/env/phase-after.png)、[裂隙预警](acceptance/horror/env/rift-warning.png)。
- [两类怪物与武器](acceptance/horror/character/after-0.png)、
  [炮台锁定](acceptance/horror/character/after-2.png)、
  [锁定与受击叠加](acceptance/horror/character/after-3.png)。
- [小窗口 READY](acceptance/horror/ui/hud-ready-800.png)、
  [低 HP 与方向反馈](acceptance/horror/ui/hud-low-hp-800.png)。
- [环境运行日志](acceptance/horror/env/validation.log)、
  [角色几何检查](acceptance/horror/character/geometry-sentry-revision2.log)、
  [角色 GL 检查](acceptance/horror/character/after.log)、
  [HUD GL 检查](acceptance/horror/ui/preview.log)。
- 正式环境、角色、像素后处理和 HUD 同管线的
  [14 张构造场景记录](acceptance/horror/presentation/capture.jsonl)，覆盖两种尺寸、
  蓄力/锁定/异变/受击/敌弹。此夹具的怪物摆位用于检查表现，不修改游戏出生位置。
- [实际游戏运行清单](acceptance/horror/runtime-manifest.json)、
  [十项流程检查](acceptance/horror/default-flow/flow.jsonl)、
  [实际 PLAYING 帧](acceptance/horror/default-flow/flow-restart.png)、
  [旧地图运行帧](acceptance/horror/legacy/playing.png)。
- [最终 Debug 构建与测试](acceptance/horror/final-build-debug.log)、
  [最终 Release 构建与测试](acceptance/horror/final-build-release.log)、
  [源码、资源与程序校验清单](acceptance/horror/game-manifest.json)。

## 运行与复现

在仓库根目录执行：

```sh
make -f Makefile.map BUILD=release -j8 all test
./silic2-release --seed 42
```

固定场景截图使用正式渲染模块和构造状态，可独立复现，不计为真人试玩：

```sh
make -f Makefile.map BUILD=release horror-preview
./bin/macos/release/horror-scene-preview /tmp/silic2-horror-preview
```

三局性能复现（独占 GPU 运行，避免同时启动编辑器或其他图形预览）：

```sh
./silic2-release --vsync off --width 1280 --height 720 --acceptance performance --output /tmp/silic2-horror-performance
```

当前完整证据位于本机 `artifacts/acceptance/horror/`；本页旁的
`docs/acceptance/horror/` 保留代表图、日志与校验信息。

## 验证边界

模块预览使用实际 GL，但角色和 HUD 输入为构造快照；游戏自动验收采用明确
标记的合成输入。尚无这版的人工键鼠可读性、主观恐怖感或耳机听感结论。
原计划五人试玩仍然待完成。运行平台为本机 M2 Mac，Windows/Linux 待验证。

第一轮改造曾沿用 `res/maps/starry_crystal_arena.json` 与 `starry_crystal`
预设标识，后续双地图更新将两种表现独立命名。没有增加新 AI、命中盒、武器能力或奖励系统。

## 双地图追加验证

已实现按地图预设独立选择环境：默认 `hollow_orbit_arena.json` 保存恐怖场景，
`starry_crystal_arena.json` 为原水晶场景；两图共用当前敌人、武器、HUD配色和音频。
34 个受保护文件的 SHA-256 与本轮开始前一致；水晶 JSON 与 `ed76aee` 逐字节相同，
恐怖 JSON 除改用 `hollow_orbit` 预设外内容一致。

两图各七个同机位场景与各自历史基线逐像素一致，覆盖异变前、中、后及裂隙提示。
同一 GL 上下文来回切换主题、暂停/重置与旧图切换通过。ATLAS 已独立复核全部
14 对 PPM，见[环境对照](acceptance/dual-arena/environment-audit.json)与
[保留文件基线](acceptance/dual-arena/preserved-baseline.json)。

| 原水晶竞技场 | 保留的太空恐怖竞技场 |
| --- | --- |
| ![水晶](acceptance/dual-arena/env/crystal/spawn-before.png) | ![太空恐怖](acceptance/dual-arena/env/hollow/spawn-before.png) |

仅分配当前主题资源，原水晶静态 VBO 为 106,512 bytes，恐怖版为 163,296 bytes；
两者仍为 11 次场景绘制及最多 2,000 粒子。水晶专属的五个 shader 与 `ed76aee`
原文件相同，共用场景加载、更新与释放流程。详细实现见
[双环境说明](../src/environment/validation/DUAL.md)。

Debug/Release 构建与测试均通过，两张地图各自实际流程 10/10、旧图运行通过，
真实音频可用。两图各14张整合GL截图无错误且冻结一致；恐怖版14张整合截图还与
拆分前逐字节一致。水晶版确认保留当前敌人和枪，见
[整合预览](acceptance/dual-arena/crystal-integrated/charge-lock-45-1280.png)。
仓库内编辑器通过 4/4 CTest，包含双预设切换、撤销/重做与保存后重新加载；
相邻旧编辑器副本未修改。命令、退出状态和程序校验见
[最终清单](acceptance/dual-arena/final-manifest.json)。

一次恐怖图首帧 READY 截图出现缺字；同一程序重检文字完整且流程10/10，未改
渲染代码。根因尚未证实，不记录为已修复；
[原图](acceptance/dual-arena/hollow-flow/flow-ready.png)与
[重检图](acceptance/dual-arena/hollow-flow-recheck/flow-ready.png)均保留。
第一轮性能数字仅描述当时记录的程序，不能作为双地图版本的新测量结果。
