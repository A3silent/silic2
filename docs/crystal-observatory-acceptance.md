# Crystal Observatory 交付与验收

2026-10-13。范围：[失焦天文馆计划](../plan/crystal-observatory.md)。
只改选中 Crystal 地图时的体验，保留此前太空恐怖和双地图工作的未提交修改。
Hollow Orbit 仍是默认入口；本轮未提交或推送。

## 可玩的内容

原 Crystal Arena 改为 48×48 连续地面的短途探索原型。低顶折线入口连接
明亮盐庭，北侧天空开口和破损子午拱建立观测设施轮廓；厚晶屏廊道逐步
露出中央悬晶。盐庭触发一次三只现有 Chaser 的有限遭遇，清除后进入安静
观察阶段。站在东侧标记内，对准三段碎弧中心持续 2.5 秒，悬晶出现内部
星点、天空和路径标记响应。随后步行到南侧出口，完成旅程。

三个碎弧是不同深度的真实几何，从观察点投影拼合；离开时因视差分开。
内部星空是表面视觉近似，不是传送门或体积折射。可走地面和静态碰撞来自
同一地图，装饰悬于通行高度之外。没有新增武器、敌人、奖励构筑或音频。
旧 Crystal 生存地图保存在 [修改前 JSON](acceptance/crystal-world/crystal-map-before.json)。

在仓库根目录运行：

```sh
make -f Makefile.map BUILD=release -j8
./silic2-release res/maps/starry_crystal_arena.json --seed 42
```

Enter 开始，WASD 移动、鼠标瞄准、左键射击。观察无需射击或交互键；注意
顶部目标提示和地面标记。Esc 暂停，结算一秒后 R 同 seed 重开。

## 设计依据与画面复核

参考研究区分了第一方事实、实际画面观察和本项目设计推导，见
[来源记录](crystal-world-references.md)。从 Eclipsium 借鉴明暗反差、轮廓、
远处地标和行为变化；从 NaissanceE 借鉴主路中的开阔/压缩空间节奏。
用户给出的 jdh 视频遇到访问验证，本轮未看指定片段，不声称复现其内容。

ATLAS 查看了真实环境和整合 GL 截图。首轮连续高墙与重复条纹过强，经过
材质尺度收敛和一次有限建筑调整，最终打开盐庭天空并加入宽大断拱。
这次完成的是可玩的空间原型，美术丰富度、探索趣味和无文字引导可读性仍需真人试玩。

![实际完整流程节点](acceptance/crystal-world/journey/contact-sheet.png)

环境探针 PNG 为实际 GL 输出，常规 1280×720，原始场景 320×200；多数构图
机位为 65 度，另保留 45 度真实玩家 FOV 机位。整合 PNG 包含真实 HUD/武器，
逻辑窗口 1280×720、Retina framebuffer 2560×1440；`*-scene.png` 是 320×200
场景读回。区别见 [环境记录](../src/environment/validation/OBSERVATORY.md)。

## 已核对的工程证据

- 独立探索控制器：Approach → Encounter → Observe → Return → Complete。
  真实脚点、视线和实体遮挡驱动观察；渲染只读取进度，清怪不直接获胜。
- Player 仅提取值式按键入口；正常 GLFW 与合成 WASD 使用同一移动/碰撞代码。
  Crystal 出生点上移 0.03，避开原有零厚地面 inclusive AABB 边界；未修改碰撞算法。
- Debug/Release 全套测试通过，编辑器 CTest 5/5 通过；构建与运行命令、二进制
  和 602 个运行/测试/编辑器源码资产哈希见 [最终清单](acceptance/crystal-world/final-manifest.json)。
  ATLAS 已读取原始测试日志并独立验证全部源码和四个二进制哈希。
- CPU 覆盖地图校验/原子加载/完整扩展 roundtrip、出生点到路线的导航、真实
  Player 输入和碰撞走全路线、错误位置/朝向/遮挡/提前离场、暂停和重置。
- 探索和 Hollow 的实际流程各 10/10：READY不计时、暂停/恢复、失焦、死亡伤害、
  结算锁和同 seed 重开。死亡流程用明确标记的合成伤害触发真实 HP/结果处理，
  不代表敌人实际攻击测试；旧 test_room 实际截图入口运行完成。
- Hollow 的另 14 张整合固定画面，ATLAS 逐一与旧基线比较，全部字节一致；
  包含两种窗口尺寸的战斗、蓄力、命中与异变阶段。
- 实际 GL 闭环 seed 42：66.160 秒、109.122 米、3 击杀、28 发/18 命中，
  无无敌、传送或强置完成；观察后实际步行入出口。此为合成输入，不是人类战斗能力。
- 碎弧投影最大误差 0.00000164 世界单位；环境冻结、重置和跨主题返回一致。
- ATLAS 直接比较 Hollow 0/47/49 秒固定机位 PPM，三张均与双地图基线逐字节一致。
  新观察状态 API 对 Hollow 无影响。[逐图结果](acceptance/crystal-world/environment/hollow-pixel-comparison.json)
- ATLAS 对本轮开始时的哈希独立核对 38 个受保护文件，敌人/角色/武器/音频、
  Hollow 地图与 shaders、共享 pixel shader 均未改变。随后独立验证 VECTOR 的
  44 文件扩展保护清单也全部相同。[保护清单](acceptance/crystal-world/protected-comparison.json)

## 性能与限制

测量来自最终地图首轮完整 GL/音频旅程，VSync off，校准视角停留 20 秒。
27,803 帧，原始测量帧累计 67.447 秒；模拟旅程 66.160 秒。截图读回/I/O帧
排除，日志记录了七次失焦后的合成恢复。ATLAS 按原始顺序 CSV 独立复算：

| 指标 | 结果 | 现有门槛 |
| --- | --- | --- |
| p95 | 5.685 ms | ≤20 ms |
| p99 | 10.749 ms | ≤33.3 ms |
| 最长连续 >50ms | 1 帧 | 不出现连续 3 帧 |
| 最大单帧 | 326.327 ms | 单独记录 |
| >50ms 帧 | 32 / 27,803 | 单独记录 |

现有门槛通过，但有明显孤立尖峰；原因尚未定位，不能推断由截图、失焦或
某个 shader 导致，也不声称全程平滑。保留首轮原始帧序，不重跑挑选最佳结果。
环境资源为 19 个地图 brush 加 3 个环境批次；环境 1,752 三角形、147,168 字节
静态 VBO、128 粒子，没有新增纹理/FBO/逐帧几何上传。这不是包含演员/HUD的总 draw call。

原始帧序、执行上下文与统计：
[完整旅程日志](acceptance/crystal-world/journey/exploration.jsonl)、
[原始 CSV](acceptance/crystal-world/journey/frames-exploration-1.csv)、
[独立复算](acceptance/crystal-world/performance-independent.json)、
[Hollow 整合画面对比](acceptance/crystal-world/hollow-fixed-comparison.json)。

## 交付边界

本轮为单个连通平面上的多节点路线，不是完整非线性世界或多层地图。
敌人仍沿现有导航追击；没有新增区域封锁或追击边界。观察交互与出口由 JSON
配置；编辑器保存、撤销和重做保留这些字段，高级参数仍需在 JSON 编辑。
当前英文 HUD 沿用项目现有风格。试玩仍需检查初次发现观察站、颜色/亮度、
迷路和回看地标的体验，以及快跑/滑铲时的可读性。
