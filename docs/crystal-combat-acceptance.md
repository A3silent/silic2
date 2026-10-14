# Crystal 立体战斗地图：实施与验收记录

用户于2026-10-13确认[方案](../plan/crystal-combat-world.md)。游戏、编辑器实现与工程验收
已完成；主观手感/趣味仍待人类试玩。未提交或推送。

## 交付内容

Crystal 现为持续交战的五区地图：中央裂环庭、下层晶壳沟谷、上层断环栈道、
倾柱侧庭与天空侧廊。受晶体侵入的轨道巨构与用途不明的异星结构共同构成轮廓。
64×64米参考底面，可见可碰撞外壳内径56米；上层6米，双坡长15米、宽6米。
7种原创OBJ模块、23模型实例、870碰撞三角形；85导航节点、171连接、41巡路点。

桥上下同处可行走，宽坡连接两层，敌人有绕行路线。保留90秒敌潮＋清场胜利、
完整小地图、既有角色/武器/音频及移动参数。侧路收益为位置、捷径、景观；
异变只改变表现。本图不再要求观察对齐或寻找出口。

地图采用schema v3和显式triangle空间模式，具体字段与资源限制见
[空间合同](crystal-spatial-contract.md)。JSON仍管理实例、参数和导航，模型存为
外部OBJ。游戏和编辑器共用加载器和空间查询，模型空腔不被包围盒填实。

![五区俯视，环境GL构图探针](acceptance/crystal-combat/environment/09-layout.png)
![普通输入实际上层路线](acceptance/crystal-combat/combat-attempt-2/combat-node-14.png)

## 工程检查

| 检查对象 | 结果与证据 |
| --- | --- |
| 坡面及分层 | 普通PlayerInput上/下坡、坡顶、滑铲、桥面/桥下同XZ独立支撑通过；[Debug](acceptance/crystal-combat/debug-build-test.log)与[Release](acceptance/crystal-combat/release-build-test.log)全测试通过 |
| 实图运动与边界 | 41/41巡路点用实际移动抵达；上层冲刺跳跃不越出可见外壳；[CPU探针](acceptance/crystal-combat/cpu-final/probe.log)零失败 |
| 空腔与射线 | 拱洞真实通行，射线/同源Sentry LOS/逐帧敌弹穿洞命中，曲外墙阻挡通过 |
| 导航 | 所有40巡路段支撑/净空验证通过；敌人绕L弯上坡追至上层，跳起目标投影仍保持正确层；无隔层接触伤害 |
| 资源事务 | 缺失资源、非法变换、凹/扭曲OBJ四边形拒绝并保留旧文档；凸共面四边形支持 |
| 地图流程 | 两轮普通移动/射击自然清场，无G、无无敌、无强制杀敌；见下表 |
| 暂停/死亡/重开 | [Crystal](acceptance/crystal-combat/crystal-flow/flow.jsonl)与[Hollow](acceptance/crystal-combat/hollow-flow/flow.jsonl)各10/10，包括暂停冻结音频、真实伤害死亡、结算输入锁、同seed重开 |
| 旧地图 | [legacy运行](acceptance/crystal-combat/legacy/runtime.log)加载与实际GL截图通过，旧探索fixture仍保留自动测试 |
| Hollow外观 | ATLAS独立比较[14张整合图](acceptance/crystal-combat/hollow-fixed/atlas-comparison.json)和[3张环境异变图](acceptance/crystal-combat/environment/hollow-comparison.json)，原始PPM全部与旧基线逐字节一致 |
| 环境状态 | [GL记录](acceptance/crystal-combat/environment/validation.log)：时间异变、冻结/重置、跨主题返回、地图数据不变、旧观察API无效均通过 |
| 保护范围 | [35项哈希对照](acceptance/crystal-combat/protected-comparison.json)：Hollow地图/shader、角色武器表现、音频、GameConfig与公共后处理均未变 |

所有运行脚本均标记synthetic_input。脚本通过PlayerInput、正常物理与普通射击，
自动选择目标和瞄准；无伤不代表人类难度。固定镜头/强置状态的截图探针仅验证
表现，不冒充实际巡路、战斗或人类试玩。

## 实战与性能

macOS M2，Release，1280×720逻辑窗口、2560×1440 framebuffer、320×200场景，
VSync off、真实音频设备。原始帧序与完整逐帧记录均归档；ATLAS独立复算分位数。

| 记录 | 自然清场 | 路程/整圈 | 击杀 | 首次开火 | p95 / p99 | 最大帧 / 连续>50ms |
| --- | ---: | --- | ---: | ---: | --- | --- |
| [首轮](acceptance/crystal-combat/combat-attempt-1/spatial-combat.jsonl) | 113.455s | 567.277m / 2 | 35 | 1.346s | 6.166 / 11.871ms | 400.270ms / 2 |
| [清场接敌脚本](acceptance/crystal-combat/combat-attempt-2/spatial-combat.jsonl) | 91.420s | 457.103m / 2 | 35 | 1.331s | 6.411 / 11.019ms | 404.594ms / 2 |

两轮均满足约定p95≤20ms、p99≤33.3ms、无连续三帧>50ms。
**约400ms的孤立停顿仍存在，原因未定位；不能据此宣称全程无卡顿。**
高峰也出现在无活敌帧，现有证据不足以归因于导航。没有删除尖峰或择优替代首轮。
性能CSV按既有协议排除截图写盘、重新聚焦和非战斗帧；`frames-complete.csv`保留这些
帧及位置/敌人数。两份CSV计时终点略有差异，最大值存在微小偏差。

首轮脚本在清场阶段继续完整巡回，出现22.002s有活敌但不可见的间隔。第二轮只改
验收输入：90秒后沿同一合法导航接近剩余敌人，正式玩法/地图未改。敌潮期间最大
间隔7.442s，清场阶段0.520s。这个指标是脚本全向LOS检测，不能替代人类视野或
证明地图节奏完美。首轮二进制未独立采样；后续验收输入修改已明确区分，最终
版本使用可核对的[交付清单](acceptance/crystal-combat/final-manifest.json)与
[源码hash](acceptance/crystal-combat/final-source-hashes.json)。ATLAS独立核对633份源码
及6份游戏/测试二进制，全部一致。

早期18敌CPU诊断曾因每敌人重复查询全图严重超时，[原始记录](acceptance/crystal-combat/cpu-first/enemy18-frames.csv)
保留。共享目标逆向距离缓存、最近可达接点和限频查询修正后，[最终CPU诊断](acceptance/crystal-combat/cpu-final/probe.log)
p95约2.04ms。CPU诊断与全游戏GL性能是不同实验，不混用其数字。

## 编辑器

使用仓库内`tools/map_editor`，不修改相邻旧编辑器。支持原生坡道、静态OBJ导入、
旋转/缩放、吸附、复制、碰撞/导航层、节点连接与诊断，保存/撤销/重做。
拖动进行轻量几何预览，松手只做一次验证提交和一次撤销记录。

Debug及RelWithDebInfo各7/7 CTest通过，最后交付binary又通过[7/7复核](acceptance/crystal-combat/editor/ctest-delivered.log)。完整85节点图的真实ImGui合成输入验证
导入、变换、复制、历史、坡道、另存外部目录并重新打开；比较全部视觉/碰撞顶点
及面保持一致。见[编辑器记录](../tools/map_editor/VALIDATION_CRYSTAL.md)和
[主GUI原始日志](acceptance/crystal-combat/editor/gui-attempt-3/run.log)。

主GUI拖动中位帧约3.16/3.33ms，松手提交整帧73.84/74.93ms，全部147帧最大240.059ms；
截图/导入/提交峰值全部保留。这是短编辑交互样本，不是游戏性能或人类可用性评价。
前两次脚本失败来自macOS全选修饰键使用错误，失败记录也归档。截图复核发现的
Inspector三分量裁切已改为标签上置、全栏输入和文字换行。
[最终布局验证](acceptance/crystal-combat/editor/layout-final/run.log)在1440和800窗口均通过，
ATLAS实际看图确认三分量与说明完整；[最终布局数据](acceptance/crystal-combat/editor/layout-final/summary.json)
两次松手整帧80.02/78.83ms，40帧最大83.52ms。首次布局脚本误把undo后的单调serial当成
连续revision，导致错误断言；[失败记录](acceptance/crystal-combat/editor/layout-attempt-1/run.log)
保留，修正断言后确认拖动中不提交、松手一次提交、一次撤销精确恢复。

![800窗口的模型旋转与拖动预览](acceptance/crystal-combat/editor/layout-final/frame-28.png)

## 复现

从仓库根目录运行：

```sh
make -f Makefile.map BUILD=release -j8 all test
./silic2-release res/maps/starry_crystal_arena.json --seed 42
./silic2-release res/maps/starry_crystal_arena.json --acceptance spatial-combat --vsync off --output /tmp/silic2-crystal-combat
./silic2-release res/maps/starry_crystal_arena.json --acceptance flow --output /tmp/silic2-crystal-flow
cmake -S tools/map_editor -B tools/map_editor/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build tools/map_editor/build -j4
ctest --test-dir tools/map_editor/build --output-on-failure
./tools/map_editor/build/map_editor res/maps/starry_crystal_arena.json
```

CPU空间探针与环境预览命令分别见构建文件和
[环境验证说明](../src/environment/validation/COMBAT.md)。编辑器完整工作流见
[使用说明](../tools/map_editor/README_CN.md)。性能检查期间不要并行启动其他GL任务。

## 已知边界

- 仅静态OBJ，实例单材质/颜色；不支持动画、MTL贴图、DCC顶点编辑或任意多边形。
  凹面/扭曲四边形需先三角化；碰撞模型要求正确朝外的面绕序。
- 导航连接需作者随地形维护，不自动生成导航网格；诊断未解决的地图不能运行。
- 编辑器为简化几何预览，最终材质、天空、敌人和音效以游戏内为准。
- 另存JSON重定位资源引用，不复制/打包模型；分享地图需要同时携带OBJ依赖。
- 人类第一次认路、近身暗色怪物辨识、战斗趣味和主观流畅度仍待实际试玩。
  不把合成瞄准的无伤结果当作难度验收。Windows/Linux实际运行也尚未验证。

最终独立核对摘要见[ATLAS复核](acceptance/crystal-combat/atlas-review.json)。
