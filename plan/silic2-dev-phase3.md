# Silic2 Phase 3 开发任务：Room Run 与受控地图生成

## 1. 阶段目标

将单竞技场扩展为 6 个节点、8-12 分钟的可重复短局，验证路线选择、战斗节奏和构筑成长能否共同工作。

这里的“自动生成地图”明确指：按 seed 生成逻辑房间图，并从经过人工验证的 JSON 房间模板池中选取完整房间。第一版在节点间卸载并加载整张地图，不运行时生成任意 brush，也不要求无缝几何拼接。

Phase 3 完成后必须具备：

- 固定 seed 的分层有向房间图。
- 战斗、精英、武器库、休整、Boss 五种节点；异常节点使用占位规则。
- 6 个可用房间模板及启动时验证。
- 进房、锁门、敌潮、清场、奖励、选路、卸载和下一房间的完整生命周期。
- 每局至少一次武器选择和一次构筑核心选择。
- 1000 个 seed 的图结构和奖励预算验证。
- 连续两局不泄漏敌人、子弹、拾取物、灯光、Augment 或 RNG 状态。

预计工作量：7-10 个有效开发日。

## 2. 进入条件

- Phase 2 的三把武器、12 个 Augment 和 RewardDraft 稳定。
- 单竞技场中至少四种构筑方向成立。
- 地图 JSON 加载、brush 碰撞、实体和灯光可正常清理并重建。
- 快速重开和 named RNG streams 已通过跨平台测试。

## 3. 非目标

- 不运行时拼接 brush、布尔几何或程序化走廊。
- 不建立完整导航网格；现有敌人继续使用简单局部移动。
- 不做地图编辑器 GUI。
- 不做无限层、回溯、大世界或房间流式并存。
- 不实现 Phase 4 的正式异常视觉和规则。
- 不根据玩家实时表现偷偷修改当前房间敌人属性。

## 4. 首版单局结构

固定六层：

```text
L0 Start
L1 Combat
L2 Elite | Armory
L3 Combat
L4 AnomalyPlaceholder | Rest
L5 BossPlaceholder
```

规则：

- L0 只选择初始武器并展示 seed，不计作战斗房。
- L1、L3 必为普通战斗。
- L2 和 L4 各生成两个候选，玩家二选一。
- L5 使用强化混合敌潮作为 Boss 占位。
- 每次只展示下一层候选，不提前公开完整图。
- 正常完成目标局时 8-12 分钟；过长先调敌潮和过渡，不增加移动速度掩盖问题。

## 5. 架构决策

### 5.1 图只描述逻辑，不拥有地图对象

`RoomGraphGenerator` 输入 run rules、模板目录摘要和 `room_graph` RNG，输出纯数据 `RoomGraph`。节点只保存 template ID、node type、encounter ID、reward profile 和边。

生成器不可加载 OpenGL 资源、不可访问当前 Player，也不根据本局战斗表现重写已生成节点。

### 5.2 完整房间切换优先

首版过渡顺序：冻结战斗 -> 保存局内快照 -> 淡出 -> 清理旧房间运行时对象 -> 加载目标 JSON -> 验证玩家安全出生 -> 生成运行时系统 -> 淡入 -> 解锁输入。

这样先验证节奏和生成约束。Socket 元数据本阶段会定义并验证，但仅用于入口/出口朝向、出生点和未来拼接兼容，不把多房间同时放在世界中。

### 5.3 RunController 拥有房间生命周期

明确状态：

```cpp
enum class RunFlowState {
    StartingRun,
    LoadingRoom,
    RoomIntro,
    Combat,
    RoomClear,
    Reward,
    RouteChoice,
    Transition,
    RunComplete,
    PlayerDead
};
```

每个状态只有一个进入和退出入口。`App` 协调更新与渲染，不通过零散 bool 决定门、奖励和敌潮。

### 5.4 模板和敌潮分别复用

RoomTemplate 描述空间能力和约束，EncounterDefinition 描述敌潮。生成时根据模板 tag 与节点风险选择兼容敌潮。禁止为每个房间复制一份只改生成点名称的敌潮。

## 6. 文件规划

新增：

```text
include/game/room_graph.h
include/game/room_graph_generator.h
include/game/room_template.h
include/game/room_catalog.h
include/game/room_transition.h
include/game/run_rules.h
include/hud/route_screen.h
src/game/room_graph.cpp
src/game/room_graph_generator.cpp
src/game/room_template.cpp
src/game/room_catalog.cpp
src/game/room_transition.cpp
src/game/run_rules.cpp
src/hud/route_screen.cpp
res/gameplay/run_rules.json
res/gameplay/rooms/index.json
res/gameplay/rooms/*.json
res/gameplay/encounters/room_*.json
tests/room_graph_test.cpp
tests/room_catalog_test.cpp
tests/run_flow_test.cpp
```

修改：

```text
include/engine/map.h
src/engine/map.cpp
include/game/run_controller.h
src/game/run_controller.cpp
include/game/encounter_director.h
src/game/encounter_director.cpp
include/engine/app.h
src/engine/app.cpp
include/hud/hud_renderer.h
src/hud/hud_renderer.cpp
Makefile.map
```

## 7. 数据规格

### 7.1 RoomTemplate 元数据

推荐在地图 JSON 顶层增加 gameplay metadata，保留现有 brushes/entities/lights：

```json
{
  "room": {
    "id": "crossfire_01",
    "tags": ["combat", "medium", "sentry_ok"],
    "difficulty": 2,
    "bounds": { "min": [-16, -2, -16], "max": [16, 10, 16] },
    "playerSpawn": "entry_a",
    "sockets": [
      { "id": "entry_a", "kind": "door", "position": [0, 1, 14], "yaw": 180 },
      { "id": "exit_a", "kind": "door", "position": [0, 1, -14], "yaw": 0 }
    ],
    "spawnZones": ["edge_north", "edge_south", "upper_ring"],
    "rewardPoint": "reward_center",
    "allowedEnemyTags": ["ground", "ranged"],
    "blockedEnemyTags": ["large"],
    "maxAlive": 36
  }
}
```

验证：唯一 ID、正 bounds、出生体积无碰撞、socket 在 bounds 内、所引用 entity/zone 存在、至少一个兼容 encounter。

### 7.2 RoomGraph

```cpp
struct RoomNode {
    uint32_t id;
    uint8_t layer;
    RoomNodeType type;
    std::string templateId;
    std::string encounterId;
    std::string rewardProfileId;
    std::vector<uint32_t> next;
};
```

序列化调试摘要必须稳定，以便同 seed 在 Mac/Windows 直接 diff。

### 7.3 RunRules

数据包含：层数、各层允许 node type、分支数、模板重复冷却、每层威胁预算、奖励预算和必保节点。生成算法不可把规则散落到 UI 或 `App`。

## 8. 生成与验证算法

生成顺序：

1. 根据层规则创建节点骨架和边。
2. 验证 Start 到 Boss 至少一条路径，且每个节点可达并能到达 Boss。
3. 为节点确定 type，应用相邻限制和必保节点。
4. 从兼容模板池加权选择 template，避免同局重复。
5. 根据模板 tag、层数和威胁预算选择 encounter。
6. 分配 reward profile，验证每条路线的武器和核心奖励保底。
7. 生成稳定摘要和 validation report。

若约束无法满足，最多用递增 attempt ID 重试 32 次。仍失败则在 Debug 构建报错，在 Release 使用内置保底图；不能输出半合法图。

## 9. 六个模板内容预算

| 模板 | 用途 | 主要空间问题 | 必要 zone |
|---|---|---|---|
| start_01 | Start/Armory | 安全选武器、学习入口 | player_safe |
| arena_ring_01 | Combat | 环形绕行、近敌拾取 | edge_any, inner_ring |
| crossfire_01 | Combat | 掩体与 Sentry 横向压制 | north, south, upper |
| pressure_pit_01 | Elite | 中心高价值目标与外圈追兵 | center, edge_any |
| shrine_split_01 | Rest/Anomaly | 明确二选一和规则提示 | player_safe |
| boss_vault_01 | Boss placeholder | 多阶段大空间考试 | boss, adds, edge_any |

一张模板可以服务多个兼容节点，但 Start 和 Boss 不与普通战斗共享模板。每张模板都需要低分辨率轮廓检查和敌人生成死角检查。

## 10. 任务拆解

### P3-01：扩展并验证 RoomTemplate 元数据

依赖：Phase 2。

工作：

- 在 Map loader 中解析 room metadata、socket、reward point 和约束。
- 将 entity name 建立为可查询但只读的索引。
- 输出独立 `RoomTemplateDescriptor`，不复制 brush 顶点。
- 为缺失、重复、越界和出生碰撞增加测试。

验收：现有旧地图仍可加载为 legacy template；正式房间缺少必填 metadata 时启动失败并定位字段。

### P3-02：建立 RoomCatalog

依赖：P3-01。

工作：

- 从 `rooms/index.json` 明确列出可用于生成的模板。
- 启动时一次性验证模板与 encounter 兼容性。
- 支持按 node type、tag、difficulty 查询候选。
- 生成内容审计表，列出没有候选的规则组合。

验收：运行时不扫描文件系统决定内容；缺少资源时在开始 run 前失败。

### P3-03：实现 RunRules 与 RoomGraph 纯数据结构

依赖：P3-02。

工作：

- 解析层、分支、相邻限制和预算。
- 实现图的稳定序列化、可达性检查和预算统计。
- 给 node ID 使用生成顺序编号，不使用内存地址。

验收：非法层、孤立节点、无 Boss 和循环图都能被测试拒绝。

### P3-04：实现 RoomGraphGenerator

依赖：P3-03。

工作：

- 使用 `room_graph` 子流，按固定算法生成骨架、类型和内容。
- 实现模板重复冷却、相邻 Rest/Boss 限制和路线保底。
- 重试使用派生 attempt seed，失败不消耗其他 RNG stream。
- 增加 CLI 或 test mode 批量导出 seed 摘要。

验收：1000 个 seed 全部合法；同 seed 输出一致；生成失败有可读约束报告。

### P3-05：明确所有系统的 room reset 合约

依赖：P3-03。

工作：

- 为 EnemyManager、Weapon、PickupManager、EncounterDirector、灯光和临时 cue 增加 `onRoomExit()`。
- 区分跨房间保留的 Player/weapon/augment/run stats 与房间局部对象。
- 清理顺序使用中央列表，避免析构回调访问已释放地图。
- Debug 构建记录过渡前后各类实体数量。

验收：加载同一房间 100 次，活动实体和 GPU 资源计数稳定。

### P3-06：实现 RoomTransition

依赖：P3-01、P3-05。

工作：

- 实现冻结、淡出、清理、加载、出生验证、淡入。
- 加载失败时回到可操作的错误状态，不能留下半加载世界。
- 过渡期间忽略射击和移动输入边沿，恢复时不产生粘键。
- 将目标 room/node/seed 写入日志。

验收：过渡时无一帧显示未加载几何；失败路径可返回主菜单或重开。

### P3-07：重构 RunController 状态机

依赖：P3-06。

工作：

- 实现 `RunFlowState` 的唯一进入/退出方法。
- 把 EncounterDirector、门、RewardScreen 和 RouteScreen 的启停放入状态转换。
- 非法转换在 Debug 断言并记录前后状态。
- 暂停作为覆盖状态，不破坏底层 run flow。

验收：自动状态测试可以从 Start 走到 RunComplete；死亡可从任意战斗状态进入并干净重开。

### P3-08：实现门锁、房间清理和节点奖励

依赖：P3-07。

工作：

- RoomIntro 后关门并开始 encounter。
- encounter 停止生成且敌人清空后进入 RoomClear。
- 按 node/reward profile 发放 Augment、武器选择、治疗或高稀有奖励。
- 明确未捡 Shard 在清场后的处理：短时自动吸取或过期，不能阻塞门。

验收：提前杀光当前敌人不会跳过 pending wave；Spawner 子体计入清场，但失效投射物不阻塞。

### P3-09：实现 RouteScreen

依赖：P3-04、P3-07。

工作：

- 展示下一层最多三个节点的类型、风险、已知奖励和已访问路径。
- 支持鼠标与数字键，选择后不可重复提交。
- 路线选择计时写入 RunStats；默认不设倒计时。
- UI 不泄露未知模板或敌潮细节。

验收：选择中位目标少于 20 秒；窗口缩放后布局和点击区域一致。

### P3-10：制作并验证六个房间模板

依赖：P3-01、P3-08。

工作：

- 从现有地图中提炼可复用 brush/material，制作六个完整 JSON。
- 每张图定义 spawn zones、出生安全体积、reward point 和 socket。
- 为三类现有敌人建立兼容/禁止标签。
- 在 1280x720 内部渲染、Retina framebuffer 和 Windows 常用分辨率下检查轮廓。

验收：没有出生即受伤、敌人卡死、不可达掉落、视线外无预警攻击或超过 8 秒找不到目标。

### P3-11：建立节点敌潮与预算曲线

依赖：P3-08、P3-10。

工作：

- 每个 Combat 模板至少两个兼容 encounter 变体。
- Elite 提高敌人职责组合，不只提高 HP。
- Boss placeholder 使用现有三敌人的两阶段组合。
- 层数只决定下一房间预算，不修改已出现敌人的生命和伤害。

验收：普通战斗 45-90 秒，整局 8-12 分钟；战斗中单段无目标时间少于 8 秒。

### P3-12：批量生成与连续运行验证

依赖：P3-04、P3-11。

工作：

- 批量生成 1000 个 seed，检查可达性、相邻限制、模板重复和奖励保底。
- 选 20 个边界 seed 跑自动状态推进。
- 人工连续完成或死亡重开至少两局，检查所有跨房间状态。
- 保存同 seed 的图、奖励和 encounter 摘要供 Mac/Windows diff。

验收：所有结构测试通过；连续两局无对象、统计或 RNG 泄漏。

## 11. 自动测试

至少覆盖：

- RoomTemplate 字段、引用、bounds 和出生验证。
- 图可达性、无孤点、无非法相邻和单 Boss 终点。
- 1000 seed 确定性生成与路线奖励预算。
- 房间状态机正常、死亡、加载失败和暂停路径。
- 清场条件对 pending wave、Spawner 子体和残留投射物的处理。
- 两局连续运行后的跨局与跨房间状态重置。

测试环境不要求创建 OpenGL context；资源计数和真实地图加载另做集成测试。

## 12. 人工测试脚本

1. 固定 seed 完整运行两次，比较节点、模板、敌潮和奖励。
2. 在每个 run flow 状态暂停、死亡或重开，检查合法恢复。
3. 选择 L2/L4 的不同路线，确认风险与奖励确实不同。
4. 在六张模板中分别测试三把武器和三种敌人。
5. 故意拖延清场、留下 Shard、让 Spawner 生成至上限，检查门逻辑。
6. 连续切换房间 100 次的开发压力测试，记录 CPU/GPU 资源计数。

记录每房时长、无目标时间、路线选择用时、未理解伤害、模板/敌潮组合和构筑形成时刻。

## 13. 建议提交边界

1. `Add validated room template metadata`
2. `Add deterministic room graph generation`
3. `Add room lifecycle and transition state machine`
4. `Add route selection and node rewards`
5. `Add six room templates and encounter variants`
6. `Add generation stress tests and run validation`

## 14. 完成定义

- 6 节点、8-12 分钟的完整 run 可以开始、完成、死亡和重开。
- 五种正式节点类型可用，异常占位节点有明确规则和奖励。
- 6 个模板通过 metadata、生成点和可读性验证。
- 1000 seed 均可达、预算合法，并满足相邻与奖励约束。
- Mac 和 Windows 同 seed 的图与敌潮稳定摘要一致。
- 两局连续运行无实体、资源、状态或随机流泄漏。

## 15. 阶段退出门

至少 80% 的内部测试局能在第三个战斗节点前形成可描述构筑，路线选择中位数低于 20 秒，且房间切换没有破坏 Pressure Arena 已有的战斗强度。满足后才进入 Phase 4；若主要问题是房间节奏或模板死角，先修内容约束，不用异常效果遮盖。
